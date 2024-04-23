#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include <string.h>
#include "mysystem.h"
#include "binary-heap.h"

//FIFO criado pelo servidor
//Cliente pode receber um sigpipe (concorrência!)
int compare(struct msg* a, struct msg* b){
	return (a->time - b->time);
}

struct msg* cloneMsg(struct msg m){
	struct msg* r = malloc(sizeof(m));

	strcpy(r->program, m.program);
	r->pid = m.pid;
	r->time = m.time;
	r->id = m.id;
	r->type = m.type;

	return r;
}

int isAvailable(int* array, int n){
	int ret = -1;

	for(int i = 0; i<n; i++){
		if(array[i]){
			ret = i;
			break;
		}
	}
	return ret;
}

int main (int argc, char * argv[]){

	int fds, fdc, fdw;
	int taskID = 1;
	int nworkers = atoi(argv[1]);
	int available[nworkers];

	for(int i = 0; i < nworkers; i++){
		available[i] = 1;
	}

	int res = mkfifo(SERVER, 0666);
    if(res<0){
        perror("erro mkfifo");
    }

	for(int i = 0; i < nworkers; i++){
		int pid;

		if((pid = fork()) == 0){
			struct msg m;
			char fifow_name[30];
			sprintf(fifow_name, WORKER "%d", i);
			int res = mkfifo(fifow_name, 0666);
			if(res<0){
				perror("erro mkfifo");
			}
			while(1){
				fdw = open(fifow_name, O_RDONLY);
				while(read(fdw, &m, sizeof(m))>0){
					int fdfile;
					char file_name[20];
					sprintf(file_name, "TASK%d.txt", m.id);
					fdfile = open(file_name, O_CREAT | O_APPEND | O_WRONLY, 0666);
					dup2(fdfile,1);
					dup2(fdfile,2);
					close(fdfile);

					mysystem(m.program);
					printf("Acabei de executar o %s\n^^TASK%d\n", m.program, m.id);
					close(1);
					close(2);
					m.type = 2;
					m.pid = i;
					fds = open(SERVER, O_WRONLY);
					write(fds, &m, sizeof(m));
					close(fds);
				}
				close(fdw);
			}
			
		}
	}

	BinaryHeap* heap = binary_heap_new(BINARY_HEAP_TYPE_MIN, compare);

	while(1){
		fds = open(SERVER, O_RDONLY);
		struct msg m;
		while(read(fds,&m,sizeof(m))>0){
			if(m.type == 0){
				m.id = taskID;
				taskID++;
				char fifoc_name[30];
				sprintf(fifoc_name, CLIENT "%d", m.pid);
				fdc = open(fifoc_name, O_WRONLY);
				write(fdc,&m,sizeof(m));
				close(fdc);
				
				if(binary_heap_num_entries(heap)){
					struct msg* r = cloneMsg(m);
					binary_heap_insert(heap, r);
				}
				else{
					int w = isAvailable(available, nworkers);
					if(w != -1){
						available[w] = 0;
						char fifow_name[30];
						sprintf(fifow_name, WORKER "%d", w);
						fdw = open(fifow_name, O_WRONLY);
						write(fdw, &m, sizeof(struct msg));
						close(fdw);
					}
					else{
						struct msg* r = cloneMsg(m);
						binary_heap_insert(heap, r);
					}
				}
				
				//printf("número de pedidos na heap: %d\n", binary_heap_num_entries(heap));
				
			}
			if(m.type == 2){
				printf("Obrigado worker%d por completares a TASK%d!\n", m.pid, m.id);
				if(binary_heap_num_entries(heap)){
					struct msg* a = binary_heap_pop(heap);
					char fifow_name[30];
					sprintf(fifow_name, WORKER "%d", m.pid);
					fdw = open(fifow_name, O_WRONLY);
					write(fdw, a, sizeof(struct msg));
					close(fdw);
				}
				else{
					available[m.pid] = 1;
				}
				
			}
		}
		close(fds);
	}
	return 0;
}

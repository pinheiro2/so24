#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include <string.h>
#include <sys/time.h>
#include "mysystem.h"
#include "binary-heap.h"
#include "execPipeline.h"

//FIFO criado pelo servidor
//Cliente pode receber um sigpipe (concorrência!)
int compare(struct msg* a, struct msg* b){
	return (a->time - b->time);
}

struct timeval clone_timeval(struct timeval t) {
    struct timeval clone;
    clone.tv_sec = t.tv_sec;
    clone.tv_usec = t.tv_usec;
    return clone;
}

struct msg* cloneMsg(struct msg m){
	struct msg* r = malloc(sizeof(m));

	strcpy(r->program, m.program);
	r->pid = m.pid;
	r->time = m.time;
	r->id = m.id;
	r->type = m.type;
	r->start_time = clone_timeval(m.start_time);
	r->end_time = clone_timeval(m.end_time);


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

long timeElapsed(struct timeval start_time, struct timeval end_time){
	long milliseconds;
	milliseconds = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                   (end_time.tv_usec - start_time.tv_usec) / 1000;
	return milliseconds;
}

void statusTasksHeap(int fdc, BinaryHeap* heap){
	int n = binary_heap_num_entries(heap);
	struct msg* auxiliar[n];
	for(int i = 0; i < n; i++){
		struct msg* a = binary_heap_pop(heap);
		write(fdc, a, sizeof(struct msg));
		auxiliar[i] = a;
	}
	for(int j = 0; j < n; j++){
		binary_heap_insert(heap, auxiliar[j]);
	}
}

int readLine(int fd, char* buffer){
	char aux[2];
	int i = 0;
	while(read(fd, aux, 1)>0){
		buffer[i] = aux[0];
		if(aux[0]=='\n'){
			buffer[i+1] = '\0';
			return 1;
		}
		i++;
	}

	return -1;

}

void statusCompleted(int fdlog, int fdc){
	char buffer[1024];
	struct msg m;
	while(readLine(fdlog, buffer) != -1){
		strcpy(m.program, buffer);
		m.type = 1;
		write(fdc, &m, sizeof(m));
	}
}

int main (int argc, char * argv[]){

	int fds, fdc, fdw, fdlog;
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
					if(m.type == 0){
						mysystem(m.program);
					}
					else{
						execPipeline(m.program);
					}
					gettimeofday(&m.end_time, NULL);
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
	struct msg* exec[nworkers];

	while(1){
		fds = open(SERVER, O_RDONLY);
		struct msg m;
		while(read(fds,&m,sizeof(m))>0){
			if(m.type == 0 || m.type == 1){
				gettimeofday(&m.start_time, NULL);
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
						exec[w] = cloneMsg(m);
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
				free(exec[m.pid]);
				if(binary_heap_num_entries(heap)){
					struct msg* a = binary_heap_pop(heap);
					char fifow_name[30];
					sprintf(fifow_name, WORKER "%d", m.pid);
					fdw = open(fifow_name, O_WRONLY);
					write(fdw, a, sizeof(struct msg));
					close(fdw);
					exec[m.pid] = cloneMsg(*a);
				}
				else{
					available[m.pid] = 1;
				}

				char line[1024];
				long realTime = timeElapsed(m.start_time, m.end_time);
				sprintf(line, "%d \"%s\" %ld ms\n", m.id, m.program, realTime);
				fdlog = open(LOG, O_CREAT | O_APPEND | O_WRONLY, 0666);
				write(fdlog, line, strlen(line));
				close(fdlog);
			}
			if(m.type == 3){
				struct msg t;
				
				t.type = 4; //tarefas em execução header

				char fifoc_name[30];
				sprintf(fifoc_name, CLIENT "%d", m.pid);
				fdc = open(fifoc_name, O_WRONLY);
				write(fdc,&t,sizeof(m));
				
				//enviar tarefas em execução 
				for(int j = 0; j < nworkers; j++){
					if(available[j] != 1){
						write(fdc, exec[j], sizeof(m));
					}
				}

				t.type = 5; //tarefas em fila de espera header
				write(fdc,&t,sizeof(m));

				statusTasksHeap(fdc, heap);

				t.type = 6; //tarefas terminadas header
				write(fdc,&t,sizeof(m));

				fdlog = open(LOG, O_CREAT | O_RDONLY, 0666);
				statusCompleted(fdlog, fdc);
				close(fdlog);

				close(fdc);
			}
		}
		close(fds);
	}
	return 0;
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include <string.h>
#include "mysystem.h"

//FIFO criado pelo servidor
//Cliente pode receber um sigpipe (concorrência!)

int main (int argc, char * argv[]){

	int fds, fdc, fdlog;
	int taskID = 1;
	

	//TODO
	int res = mkfifo(SERVER, 0666);
    if(res<0){
        perror("erro mkfifo");
    }

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

				if(fork()==0){
					int ret = mysystem(m.program);
					_exit(ret);
				}
				
			}
		}
		close(fds);
	}
	return 0;
}

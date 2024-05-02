#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include <string.h>

int main (int argc, char * argv[]){

	/*
	if (argc < 5) {
		printf("Missing argument.\n");
		_exit(1);
	}*/
	int fdc, fds;
	char fifoc_name[30];
	sprintf(fifoc_name, CLIENT "%d", getpid());
	mkfifo(fifoc_name, 0666);
	
	struct msg m;
	if(argc == 5){
		m.time = atoi(argv[2]);
		m.pid = getpid();
		strcpy(m.program, argv[4]);
		if(strcmp(argv[3], "-u")==0){
			m.type = 0;
		}
		if(strcmp(argv[3], "-p")==0){
			m.type = 1;
		}
		m.id = -1;
		fds = open(SERVER, O_WRONLY);
		write(fds, &m, sizeof(m));
		close(fds);
		
		
		fdc = open(fifoc_name, O_RDONLY);
		read(fdc, &m, sizeof(m));
		char line[1024];
		sprintf(line, "TASK %d Received\n", m.id);
		write(1, line, strlen(line));
		close(fdc);
		
	}

	if(argc == 2){
		if(strcmp(argv[1], "status")==0){
			m.type = 3;
			m.pid = getpid();
			fds = open(SERVER, O_WRONLY);
			write(fds, &m, sizeof(m));
			close(fds);


			int mode = -1; //flag que identifica o momento a qual uma task se refere
			fdc = open(fifoc_name, O_RDONLY);
			while(read(fdc, &m, sizeof(m))>0){
				char line[1024];
				if(m.type == 4){
					mode = 4;
					sprintf(line, "Executing\n");
				}
				else{
					if(m.type == 5){
						mode = 5;
						sprintf(line, "\nScheduled\n");
					}
					else{
						if(m.type == 6){
							mode = 6;
							sprintf(line, "\nCompleted\n");
						}
						else{
							if (mode!=6){
								sprintf(line, "%d \"%s\"\n", m.id, m.program);
							}
							else{
								//sprintf(line, "%d \"%s\" %d ms\n", m.id, m.program, m.time);
								sprintf(line, "%s", m.program);
							}
						}
					}
				}
				
				
				
				write(1, line, strlen(line));
				
			}
			close(fdc);

		}
	}
	unlink(fifoc_name);
	
	return 0;
}


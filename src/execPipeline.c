#include "execPipeline.h"

int exec_command(char* arg){

	//Estamos a assumir numero maximo de argumentos
	char *exec_args[10];

	char *string;	
	int exec_ret = 0;
	int i=0;

	char* command = strdup(arg);

	string=strtok(command," ");
	
	while(string!=NULL){
		exec_args[i]=string;
		string=strtok(NULL," ");
		i++;
	}

	exec_args[i]=NULL;
	
	exec_ret=execvp(exec_args[0],exec_args);
	
	return exec_ret;
}

int execPipeline(char* program){
	char *arg = strdup(program);

	int ncmds=0;
	char *commands[10];

	char *string;	
	int j=0;

	char* command = strdup(arg);

	string=strtok(command,"|");
	
	while(string!=NULL){
		commands[j]=string;
		string=strtok(NULL,"|");
		j++;
		ncmds++;
	}

	int pipes[ncmds-1][2];
	
	for(int i=0; i<ncmds; i++){
		if(i==0){
			pipe(pipes[i]);
			switch(fork()){
				case 0:
					close(pipes[i][0]);
					dup2(pipes[i][1], 1);
					close(pipes[i][1]);
					exec_command(commands[i]);
					break;

				default:
					close(pipes[i][1]);	
			}
		}
		else{
			if(i==ncmds-1){
				switch(fork()){
					case 0:
						close(pipes[i][0]);
						dup2(pipes[i-1][0], 0);
						close(pipes[i-1][0]);
						exec_command(commands[i]);
						break;

					default:
						close(pipes[i-1][0]);	
				}
			}
			else{
				pipe(pipes[i]);
				switch(fork()){
					case 0:
						close(pipes[i][0]);

						dup2(pipes[i-1][0], 0);
						close(pipes[i-1][0]);

						dup2(pipes[i][1], 1);
						close(pipes[i][1]);

						exec_command(commands[i]);
						break;

					default:
						close(pipes[i][1]);
						close(pipes[i-1][0]);	
				}
			}
		}
	}

	for(int i=0; i<ncmds; i++){
		wait(NULL);
	}

	return 0;
}
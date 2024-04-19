#include "mysystem.h"



// recebe um comando por argumento
// returna -1 se o fork falhar
// caso contrario retorna o valor do comando executado
int mysystem (const char* command) {
	int status, ret;
	char* arr[50];
	int i = 0;
	int res = -1;
	char *token, *string, *tofree;

	tofree = string = strdup(command);

	while((token = strsep(&string, " ")) != NULL){
		arr[i] = token;
		i++;
	}

	arr[i] = NULL;
	//printf("%s\n", arr[0]);

	pid_t pid = fork();
    switch(pid){
        case -1:
            perror("erro fork");
            break;

        case 0:
            ret = execvp(arr[0], arr);
			_exit(ret);
            break;

        default:
            wait(&status);
            if(WIFEXITED(status)){
                //printf("filho terminou com %d\n", WEXITSTATUS(status));
				res = WEXITSTATUS(status);
				if(res == 255) res = -1;
            }
            else{
                //printf("filho não terminou\n");
            }
    }

	free(tofree);
	return res;
}
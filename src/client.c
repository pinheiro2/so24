#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include <string.h>

int main (int argc, char * argv[]){

	if (argc < 5) {
		printf("Missing argument.\n");
		_exit(1);
	}
	int fdc, fds;
	char fifoc_name[30];
	sprintf(fifoc_name, CLIENT "%d", getpid());
	mkfifo(fifoc_name, 0666);
	
	struct msg m;
	m.time = atoi(argv[2]);
	m.pid = getpid();
	strcpy(m.program, argv[4]);
	m.type = 0;
	m.id = -1;
	fds = open(SERVER, O_WRONLY);
	write(fds, &m, sizeof(m));
	close(fds);
	
	
	fdc = open(fifoc_name, O_RDONLY);
	read(fdc, &m, sizeof(m));
	printf("TASK %d\n", m.id);
	close(fdc);
	unlink(fifoc_name);
	
	return 0;
}


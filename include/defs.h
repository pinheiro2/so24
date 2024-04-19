
#define SERVER "fifo_server"
#define CLIENT "fifo_client"
#define WORKER "fifo_worker"

typedef struct msg{
    char program[100];
    int pid;
    int time;
    int id;
    int type;
} Msg;
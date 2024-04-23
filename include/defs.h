
#define SERVER "fifo_server"
#define CLIENT "fifo_client"
#define WORKER "fifo_worker"
#define LOG "../tmp/log.txt"

typedef struct msg{
    char program[100];
    int pid;
    int time;
    int id;
    int type;
    struct timeval start_time;
    struct timeval end_time;
} Msg;
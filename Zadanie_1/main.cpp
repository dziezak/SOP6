#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/wait.h>

#define QUEUE_NAME_S "/queue_s"
#define QUEUE_NAME_D "/queue_d"
#define QUEUE_NAME_M "/queue_m"
#define CLIENT_QUEUE_TEMPLATE "/client_queue_%d"
#define PERMISSIONS 0666

void server() {
    mqd_t mq_s, mq_d, mq_m;
    struct mq_attr attr = {0, 10, sizeof(int), 0};
    
    mq_s = mq_open(QUEUE_NAME_S, O_CREAT | O_RDONLY, PERMISSIONS, &attr);
    mq_d = mq_open(QUEUE_NAME_D, O_CREAT | O_RDONLY, PERMISSIONS, &attr);
    mq_m = mq_open(QUEUE_NAME_M, O_CREAT | O_RDONLY, PERMISSIONS, &attr);

    if (mq_s == (mqd_t)-1 || mq_d == (mqd_t)-1 || mq_m == (mqd_t)-1) {
        perror("mq_open failed");
        exit(1);
    }
    
    printf("Serwer utworzył kolejki: %s, %s, %s\n", QUEUE_NAME_S, QUEUE_NAME_D, QUEUE_NAME_M);
    
    sleep(1);
    
    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);
    mq_unlink(QUEUE_NAME_S);
    mq_unlink(QUEUE_NAME_D);
    mq_unlink(QUEUE_NAME_M);
    
    printf("Serwer usunął kolejki i kończy działanie.\n");
}

void client() {
    mqd_t mq;
    char client_queue_name[50];
    snprintf(client_queue_name, sizeof(client_queue_name), CLIENT_QUEUE_TEMPLATE, getpid());
    
    struct mq_attr attr = {0, 10, sizeof(int), 0};
    mq = mq_open(client_queue_name, O_CREAT | O_RDONLY, PERMISSIONS, &attr);

    if (mq == (mqd_t)-1) {
        perror("mq_open failed");
        exit(1);
    }
    
    printf("Klient (PID: %d) utworzył kolejkę: %s\n", getpid(), client_queue_name);
    
    sleep(1);
    
    mq_close(mq);
    mq_unlink(client_queue_name);
    printf("Klient (PID: %d) usunął kolejkę i kończy działanie.\n", getpid());
}

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        client();
    } else {
        server();
        wait(NULL);
    }
    
    return 0;
}

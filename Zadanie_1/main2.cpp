
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <mqueue.h>

#define QUEUE_NAME_S "/queue_s"
#define QUEUE_NAME_D "/queue_d"
#define QUEUE_NAME_M "/queue_m"
#define CLIENT_QUEUE_TEMPLATE "/client_queue_%d"
#define PERMISSIONS 0666
#define MAX_MSG_SIZE 256

typedef struct {
    pid_t client_pid;
    int num1;
    int num2;
} message_t;

void server() {
    mqd_t mq_s, mq_d, mq_m;
    struct mq_attr attr = {0, 10, sizeof(message_t), 0};
    
    mq_s = mq_open(QUEUE_NAME_S, O_CREAT | O_RDONLY, PERMISSIONS, &attr);
    mq_d = mq_open(QUEUE_NAME_D, O_CREAT | O_RDONLY, PERMISSIONS, &attr);
    mq_m = mq_open(QUEUE_NAME_M, O_CREAT | O_RDONLY, PERMISSIONS, &attr);
    if (mq_s == (mqd_t)-1 || mq_d == (mqd_t)-1 || mq_m == (mqd_t)-1) {
        perror("mq_open failed");
        exit(1);
    }
    
    printf("Serwer utworzył kolejki: %s, %s, %s\n", QUEUE_NAME_S, QUEUE_NAME_D, QUEUE_NAME_M);
    
    message_t msg;
    mqd_t queues[] = {mq_s, mq_d, mq_m};
    char *queue_names[] = {QUEUE_NAME_S, QUEUE_NAME_D, QUEUE_NAME_M};
    
    for (int i = 0; i < 3; i++) {
        if (mq_receive(queues[i], (char*)&msg, sizeof(msg), NULL) > 0) {
            printf("Serwer otrzymał wiadomość od PID %d: %d, %d z %s\n", msg.client_pid, msg.num1, msg.num2, queue_names[i]);
            
            char client_queue_name[50];
            snprintf(client_queue_name, sizeof(client_queue_name), CLIENT_QUEUE_TEMPLATE, msg.client_pid);
            mqd_t client_mq = mq_open(client_queue_name, O_WRONLY);
            if (client_mq != (mqd_t)-1) {
                int result = 0;
                if (i == 0) result = msg.num1 + msg.num2;
                else if (i == 1) result = (msg.num2 != 0) ? (msg.num1 / msg.num2) : 0;
                else if (i == 2) result = msg.num1 * msg.num2;
                
                mq_send(client_mq, (char*)&result, sizeof(result), 0);
                mq_close(client_mq);
            }
        }
    }
    
    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);
    mq_unlink(QUEUE_NAME_S);
    mq_unlink(QUEUE_NAME_D);
    mq_unlink(QUEUE_NAME_M);
    printf("Serwer usunął kolejki i kończy działanie.\n");
}

void client(char *queue_name) {
    mqd_t mq, server_mq;
    char client_queue_name[50];
    snprintf(client_queue_name, sizeof(client_queue_name), CLIENT_QUEUE_TEMPLATE, getpid());
    
    struct mq_attr attr = {0, 10, sizeof(int), 0};
    mq = mq_open(client_queue_name, O_CREAT | O_RDONLY, PERMISSIONS, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open failed");
        exit(1);
    }
    
    server_mq = mq_open(queue_name, O_WRONLY);
    if (server_mq == (mqd_t)-1) {
        perror("mq_open server queue failed");
        exit(1);
    }
    
    int num1, num2;
    printf("Podaj dwie liczby: ");
    scanf("%d %d", &num1, &num2);
    
    message_t msg = {getpid(), num1, num2};
    mq_send(server_mq, (char*)&msg, sizeof(msg), 0);
    
    int result;
    if (mq_receive(mq, (char*)&result, sizeof(result), NULL) > 0) {
        printf("Klient (PID: %d) otrzymał wynik: %d\n", getpid(), result);
    }
    
    mq_close(mq);
    mq_unlink(client_queue_name);
    printf("Klient (PID: %d) usunął kolejkę i kończy działanie.\n", getpid());
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s <queue_name>\n", argv[0]);
        exit(1);
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        client(argv[1]);
    } else {
        server();
        wait(NULL);
    }
    
    return 0;
}

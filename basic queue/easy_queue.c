#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define QUEUE_NAME "/moja_kolejka"
#define MAX_SIZE 1024

int main() {
    mqd_t mq;
    struct mq_attr attr;

    // Ustawienia kolejki
    attr.mq_flags = 0;              // tryb blokujacy
    attr.mq_maxmsg = 10;            // Maksymalna liczba wiadomosci
    attr.mq_msgsize = MAX_SIZE;     // Maksymalny rozmiar wiadomosci 
    attr.mq_curmsgs = 0;            // Liczba aktualnych wiadomosci
    // Tworzymy kolejkę przed fork()
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if(mq == -1){
        perror("Blad tworzenia kolejki");
        exit(1);
    }

    pid_t pid = fork();

    if(pid == 0){
        sleep(1);
        const char* message = "Hello from child!";
        if(mq_send(mq, message, strlen(message)+1, 0) == -1){
            perror("Blad wysylania wiadomosci");
        }else{
            printf("[Potomek]: Wyslalem wiadomosci\n");
        }
    }else{
        char buffer[MAX_SIZE];
        if(mq_receive(mq, buffer, MAX_SIZE, NULL) == -1){
            perror("Blad odbierania wiadomosci");
        }else{
            printf("[Rodzic]: Odebralem wiadomosc: %s\n", buffer);
        }
        mq_unlink(QUEUE_NAME);
    }

    mq_close(mq);
    return 0;
}

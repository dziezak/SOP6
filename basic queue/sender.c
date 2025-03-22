#include <stdio.h>
#include <stdlib.h>
//#include <sys/stat.h>
#include <mqueue.h>
#include <string.h>

#define QUEUE_NAME "/moja_kolejka"

int main(){
    mqd_t mq;
    const char* message = "Wiadomosc testowa";
    mq = mq_open(QUEUE_NAME, O_WRONLY);
    if(mq == -1){
        perror("Blad otwierania kolejki");
        exit(1);
    }
    if(mq_send(mq, message, strlen(message) + 1, 0) == -1){
        perror("Blad wysylania wiadomosci");
    }
    mq_close(mq);
    return 0;
}
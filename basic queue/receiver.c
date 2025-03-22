#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>

#define QUEUE_NAME "/moja_kolejka"
#define MAX_SIZE 1024

int main(){
    mqd_t mq;
    char buffer[MAX_SIZE];

    mq = mq_open(QUEUE_NAME, O_RDONLY);
    if(mq == -1){
        perror("Blad otwierania kolejki");
        exit(1);
    }
    if(mq_receive(mq, buffer, MAX_SIZE, NULL) == -1){
        perror("Blad odbierania wiadomosci");
    }else{
        printf("Odebrano: %s \n", buffer);
    }
    mq_close(mq);
    return 0;
}
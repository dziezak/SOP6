#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <time.h>

#define TASK_QUEUE_NAME "/task_queue_%d"
#define RESULT_QUEUE_NAME "/result_queue_%d_%d"
#define TASK_PER_WORKER 5

typedef struct {
    float num1;
    float num2;
} Task;

float get_random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

int get_random_time(int min, int max) {
    return min + rand() % (max - min + 1);
}

void worker(int worker_id);  // Forward declaration

void server(int n_workers) {
    char task_queue_name[256];
    sprintf(task_queue_name, TASK_QUEUE_NAME, getpid());

    struct mq_attr attr = {0, 10, sizeof(Task), 0};
    mqd_t task_queue = mq_open(task_queue_name, O_CREAT | O_WRONLY, 0644, &attr);
    if (task_queue == -1) {
        perror("mq_open (server)");
        exit(EXIT_FAILURE);
    }

    printf("Server is starting...\n");

    for (int i = 0; i < n_workers; i++) {
        if (fork() == 0) {
            worker(i);
            exit(0);
        }
    }

    srand(time(NULL));
    int numer_of_tasks = 0;
    int total_tasks = 5 * n_workers;
    //while (numer_of_tasks < n_workers) {
    for(int i=0; i<total_tasks; i++)
    {
        Task task = {get_random_float(0.0, 100.0), get_random_float(0.0, 100.0)};
        if (mq_send(task_queue, (char*)&task, sizeof(Task), 0) == -1) {
            //perror("mq_send (server)");
            printf("Queue is full!\n");
        }else{
            printf("New task queued: %.2f + %.2f\n", task.num1, task.num2);
        }
        usleep(get_random_time(100000, 500000)); 
        numer_of_tasks++;
    }

    while (wait(NULL) > 0);
    mq_close(task_queue);
    mq_unlink(task_queue_name);

    printf("All child processes have finished.\n");
}

void worker(int worker_id) {
    char task_queue_name[256];
    //char result_queue_name[256];

    sprintf(task_queue_name, TASK_QUEUE_NAME, getppid());
    //sprintf(result_queue_name, RESULT_QUEUE_NAME, getppid(), worker_id);

    struct mq_attr result_queue_attr = {0, 10, sizeof(float), 0};
    mqd_t task_queue = mq_open(task_queue_name, O_RDONLY);
    if (task_queue == -1) {
        perror("mq_open (worker task)");
        exit(EXIT_FAILURE);
    }

    // mqd_t result_queue = mq_open(result_queue_name, O_CREAT | O_WRONLY, 0644, &result_queue_attr);
    // if (result_queue == -1) {
    //     perror("mq_open (worker result)");
    //     exit(EXIT_FAILURE);
    // }

    printf("[%d] Worker ready!\n", getpid());

    Task task;
    for(int i=0; i<TASK_PER_WORKER; i++){
        if (mq_receive(task_queue, (char*)&task, sizeof(Task), NULL) > 0) {
            float result = task.num1 + task.num2;
            usleep(get_random_time(500000, 2000000));

            //if (mq_send(result_queue, (char*)&result, sizeof(float), 0) == -1) {
            //    perror("mq_send (worker result)");
            //}
            printf("[%d] Task %.2f + %.2f = %.2f\n", getpid(), task.num1, task.num2, result);
        }
    }

    printf("[%d] Exits!\n", getpid());
    mq_close(task_queue);
    //mq_close(result_queue);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_workers>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_workers = atoi(argv[1]);
    if (n_workers < 2 || n_workers > 20) {
        fprintf(stderr, "Number of workers must be between 2 and 20.\n");
        exit(EXIT_FAILURE);
    }

    server(n_workers);

    return 0;
}


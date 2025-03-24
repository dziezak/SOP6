#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define MAX_CHILDREN 100
#define MAX_NAME_LENGTH 255

void server(int n_children, char *message) {
    if (n_children == 0) {
        printf("No children to play.\n");
        return;
    }

    // Tworzymy kolejkę komunikatów dla pierwszego dziecka
    char queue_name[256];
    sprintf(queue_name, "sop_cwg_%d", getpid());

    struct mq_attr attr = {0, 2, sizeof(char*), 0};
    mqd_t first_queue = mq_open(queue_name, O_CREAT | O_WRONLY, 0644, &attr);
    if (first_queue == -1) {
        perror("mq_open (server)");
        exit(EXIT_FAILURE);
    }

    // Tworzymy dzieci
    for (int i = 0; i < n_children; i++) {
        if (fork() == 0) {
            child(i, i == 0 ? getpid() : getppid(), message); // Pierwsze dziecko ma koordynatora jako poprzedniego
            exit(0);
        }
    }

    // Oczekiwanie na zakończenie wszystkich dzieci
    while (wait(NULL) > 0);

    mq_close(first_queue);
    mq_unlink(queue_name);
    printf("All children have finished the game.\n");
}

void child(int child_id, pid_t previous_pid, char *message) {
    char queue_name[256];
    sprintf(queue_name, "sop_cwg_%d", getpid());

    struct mq_attr attr = {0, 2, sizeof(char*), 0};
    mqd_t my_queue = mq_open(queue_name, O_CREAT | O_RDONLY | O_WRONLY, 0644, &attr);
    if (my_queue == -1) {
        perror("mq_open (child)");
        exit(EXIT_FAILURE);
    }

    // Jeśli dziecko nie jest pierwsze, odbieramy wiadomość od poprzedniego dziecka
    mqd_t previous_queue;
    if (child_id > 0) {
        char prev_queue_name[256];
        sprintf(prev_queue_name, "sop_cwg_%d", previous_pid);
        previous_queue = mq_open(prev_queue_name, O_RDONLY);
        if (previous_queue == -1) {
            perror("mq_open (previous queue)");
            exit(EXIT_FAILURE);
        }
    } else {
        previous_queue = my_queue; // Pierwsze dziecko otrzymuje wiadomość od koordynatora
    }

    // Rozbijamy wiadomość na słowa
    char *word = strtok(message, " ");
    while (word != NULL) {
        printf("[%d] %s got the message: '%s'\n", getpid(), children[child_id], word);

        if (child_id < n_children - 1) {
            // Przekazujemy słowo do kolejnego dziecka
            if (mq_send(my_queue, word, sizeof(word), 0) == -1) {
                perror("mq_send (child)");
                exit(EXIT_FAILURE);
            }
        }

        // Odbieramy następne słowo
        word = strtok(NULL, " ");
    }

    // Dziecko kończy pracę
    printf("[%d] %s has left the game!\n", getpid(), children[child_id]);

    // Zamykamy kolejki
    mq_close(my_queue);
    if (previous_queue != my_queue) {
        mq_close(previous_queue);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s P T1 T2\n", argv[0]);
        return EXIT_FAILURE;
    }

    int P = atoi(argv[1]);
    int T1 = atoi(argv[2]);
    int T2 = atoi(argv[3]);

    if (P < 0 || P > 100 || T1 < 100 || T2 > 6000 || T1 >= T2) {
        fprintf(stderr, "Invalid parameters. Ensure: 0 ≤ P ≤ 100 and 100 ≤ T1 < T2 ≤ 6000.\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    char children[MAX_CHILDREN][MAX_NAME_LENGTH];
    int num_children = 0;

    while (num_children < MAX_CHILDREN && fgets(children[num_children], MAX_NAME_LENGTH, stdin)) {
        size_t len = strlen(children[num_children]);
        if (len > 0 && children[num_children][len - 1] == '\n') {
            children[num_children][len - 1] = '\0';
        }
        num_children++;
    }

    if (num_children == 0) {
        fprintf(stderr, "No children entered! Exiting...\n");
        return EXIT_FAILURE;
    }

    // Zaczynamy grę
    char message[255];
    printf("Enter the message to pass (start <message>): ");
    if (fgets(message, sizeof(message), stdin) == NULL) {
        fprintf(stderr, "Error reading message.\n");
        exit(EXIT_FAILURE);
    }
    
    // Usuwamy ewentualny znak nowej linii z końca wiadomości
    size_t len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
        message[len - 1] = '\0';
    }

    // Koordynator wywołuje funkcję serwera
    server(num_children, message);
    
    return EXIT_SUCCESS;
}

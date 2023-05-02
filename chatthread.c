#include <pthread.h>
#include <stdbool.h>
#include "codec.h"
#include "myArrayList.c"
#include<unistd.h>

#define CHUNK_SIZE 1024
#define ENCRYPT 0
#define DECRYPT 1

int argc1;
char **argv1;

arraylist array;
bool reading_complete = false;

pthread_cond_t task_available = PTHREAD_COND_INITIALIZER;
int key;
int encryptOrDecrypt;

typedef struct {
    item *head;
    item *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_cond_t queue_empty; // New condition variable
} queue;

queue *create_queue() {
    queue *q = (queue *) malloc(sizeof(queue));
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    pthread_cond_init(&q->queue_empty, NULL); // Initialize the new condition variable
    return q;
}

bool isEmpty(queue *q) {
    return q->head == NULL;
}

void enqueue(queue *q, const char *in, int index) {
    item *new_item = malloc(sizeof(item));
    new_item->input = malloc(strlen(in) + 1);
    new_item->output = malloc(strlen(in) + 1);
    new_item->index = index;
    strcpy(new_item->input, in);
    strcpy(new_item->output, in); // later needs to be from the encrypt decrypt

    pthread_mutex_lock(&q->mutex);
    if (q->tail) {
        q->tail->next = new_item;
    } else {
        q->head = new_item;
    }
    q->tail = new_item;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

item *dequeue(queue *q) {
    pthread_mutex_lock(&q->mutex);
    while (!q->head) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    item *dequeued = q->head;
    q->head = q->head->next;
    if (!q->head) {
        q->tail = NULL;
    }
    pthread_mutex_unlock(&q->mutex);
    return dequeued;
}

typedef struct {
    queue *task_queue;
} thread_pool;

thread_pool pool;
int toPrintIndex = 0;
int currIndex = 0;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER; // Add print_mutex

void *worker_function(void *arg) {
    while (true) {
        pthread_mutex_lock(&print_mutex);
        while (!reading_complete && isEmpty(pool.task_queue)) {
            pthread_cond_wait(&task_available, &print_mutex);
        }
        if (reading_complete && isEmpty(pool.task_queue)) {
            pthread_mutex_unlock(&print_mutex);
            break;
        }
        item *task = dequeue(pool.task_queue);
        printf("\n*** %d ****\n",task->index);
        pthread_mutex_unlock(&print_mutex);

        if (encryptOrDecrypt == ENCRYPT) {
            encrypt(task->output, key);
        } else {
            decrypt(task->output, key);
        }

        pthread_mutex_lock(&print_mutex);
        if (toPrintIndex == task->index) {
            printf("%s", task->output);
            toPrintIndex++;
        } else {
            add_at(&array, task, task->index);
            item *currItem;
            currItem = get(&array, toPrintIndex);
            while (currItem != NULL) {
                printf("%s", currItem->output);
                toPrintIndex++;
                currItem = get(&array, toPrintIndex);
            }
        }
        pthread_cond_signal(&pool.task_queue->queue_empty); // Signal the reader thread
        pthread_mutex_unlock(&print_mutex);
    }
    return NULL;
}

bool read_and_process(FILE *input) {
    char buffer[CHUNK_SIZE + 1]; // Add space for null terminator
    size_t bytes_read;

    while ((bytes_read = fread(buffer, sizeof(char), CHUNK_SIZE, input)) > 0) {
        buffer[bytes_read] = '\0'; // Add null terminator to the buffer
        enqueue(pool.task_queue, buffer, currIndex);
        currIndex++;
        pthread_cond_signal(&task_available); // Signal worker threads

        // Wait for at least one worker thread to process the data
        pthread_mutex_lock(&print_mutex);
        pthread_cond_wait(&pool.task_queue->queue_empty, &print_mutex);
        pthread_mutex_unlock(&print_mutex);
    }

    return !ferror(input);
}

void *readerThreadFunction(void *arg) {
    if (argc1 < 4 || strcmp(argv1[3], "-") == 0) {
        if (!read_and_process(stdin)) {
            printf("Error reading from stdin\n");
            return NULL;
        }
    } else {
        FILE *input = fopen(argv1[3], "r");
        if (!input) {
            printf("Error opening input file\n");
            return NULL;
        }

        if (!read_and_process(input)) {
            printf("Error reading from input file\n");
            fclose(input);
            return NULL;
        }
        fclose(input);
    }

    pthread_mutex_lock(&print_mutex);
    reading_complete = true;
    pthread_cond_broadcast(&task_available); // Signal worker threads
    pthread_mutex_unlock(&print_mutex);

    return NULL;
}

int main(int argc, char **argv) {
    clock_t t;
    t = clock();


    if (argc >= 2) {
        key = atoi(argv[1]);
        if (strcmp(argv[2], "-e") == 0) {
            encryptOrDecrypt = ENCRYPT;
        } else if (strcmp(argv[2], "-d") == 0) {
            encryptOrDecrypt = DECRYPT;
        }
    }

    argc1 = argc;
    argv1 = argv;
    printf("started\n");
    initArrayList(&array);
    int num_threads = 20;
    pthread_t threads[num_threads];
    pool.task_queue = create_queue();

    pthread_t readerThread;
    pthread_create(&readerThread, NULL, readerThreadFunction, NULL);

    // sleep(5);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_function, &pool);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(readerThread, NULL); // Wait for readerThread to finish
    printf("\n");


    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
    printf("%f\n", time_taken);

    return 0;
}
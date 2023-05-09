#include <pthread.h>
#include <stdbool.h>
#include "codec.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 1024
#define ENCRYPT 0
#define DECRYPT 1
#define INITIAL_CAPACITY 10


typedef struct item {
    char *input;
    char *output;
    int index;
    struct item *next;
} item;

typedef struct {
    item *array;
    size_t size;
    size_t capacity;
} arraylist;

void initArrayList(arraylist *al) {
    al->size = 0;
    al->capacity = INITIAL_CAPACITY;
    al->array = malloc(al->capacity * sizeof(item));
}

void add(arraylist *al, item *it) {
    if (al->size == al->capacity) {
        al->capacity *= 2;
        al->array = realloc(al->array, al->capacity * sizeof(item));
    }
    al->array[al->size++] = *it;
}

void add_at(arraylist *al, item *currItem, size_t index) {
    if (index > al->size) {
        // Resize the arraylist to accommodate the given index
        size_t new_capacity = index + 1;
        if (new_capacity < al->capacity) {
            new_capacity = al->capacity * 2;
        }
        al->array = realloc(al->array, new_capacity * sizeof(item));
        al->capacity = new_capacity;
        al->size = index + 1;
    } else if (al->size == al->capacity) {
        // Resize the arraylist if it is at capacity
        al->capacity *= 2;
        al->array = realloc(al->array, al->capacity * sizeof(item));
    }

    // Create a new item and copy the data
    item newItem;
    newItem.input = malloc(strlen(currItem->input) + 1);
    newItem.output = malloc(strlen(currItem->output) + 1);
    newItem.index = currItem->index;
    strcpy(newItem.input, currItem->input);
    strcpy(newItem.output, currItem->output);

    al->array[index] = newItem;
    al->size++;
}


item *get(arraylist *al, size_t index) {
    if (index >= al->size) {
        return NULL;
    }
    return &al->array[index];
}

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
    new_item->next = NULL; // Initialize the next pointer to NULL
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
int currIndex = 0;

void *worker_function(void *arg) {
    while (true) {
        if (reading_complete && isEmpty(pool.task_queue)) {
            break;
        }
        item *task = dequeue(pool.task_queue);

        if (encryptOrDecrypt == ENCRYPT) {
            encrypt(task->output, key);
        } else {
            decrypt(task->output, key);
        }
        add_at(&array, task, task->index);

        // Free the memory for the dequeued item
        free(task->input);
        free(task->output);
        free(task);

        pthread_cond_signal(&pool.task_queue->queue_empty); // Signal the reader thread
    }
    // printf("worker function finished\n");
    return NULL;
}

bool read_and_process(FILE *input) {
    char buffer[CHUNK_SIZE]; // Add space for null terminator
    size_t bytes_read;

    while ((bytes_read = fread(buffer, sizeof(char), CHUNK_SIZE - 1, input)) > 0) {
        buffer[bytes_read] = '\0'; // Add null terminator to the buffer
        enqueue(pool.task_queue, buffer, currIndex);
        // printf("added %d to the queue\n",currIndex);
        currIndex++;
        pthread_cond_signal(&task_available); // Signal worker threads
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

    reading_complete = true;
    pthread_cond_broadcast(&task_available); // Signal worker threads


    return NULL;
}

void *printerThreadFunction(void *arg) {

    int toprint = 0;
    sleep(5);
    while (!reading_complete) {
        if (toprint < currIndex) {
            item *it = get(&array, toprint);
            if (it != NULL && it->output != NULL) {
                printf("%s", it->output);
                toprint++;
            } else {
                sleep(1);
            }
        }
    }

    while (toprint < currIndex) {
        item *it = get(&array, toprint);
        if (it != NULL && it->output != NULL) {
            printf("%s", it->output);
            toprint++;
        } else {
            sleep(1);
        }
    }
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
    initArrayList(&array);
    int num_threads = 20;
    pthread_t threads[num_threads];
    pool.task_queue = create_queue();

    pthread_t readerThread;
    pthread_create(&readerThread, NULL, readerThreadFunction, NULL);

    sleep(1);
    if (reading_complete) {
        if (currIndex == 1) {
            num_threads = 1;
        } else if (currIndex < num_threads) {
            num_threads = currIndex - 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_function, &pool);
    }


    pthread_t printerThread;
    pthread_create(&printerThread, NULL, printerThreadFunction, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }


    pthread_join(readerThread, NULL); // Wait for readerThread to finish
    pthread_join(printerThread, NULL); // Wait for printerThread to finish

    printf("\n");

    t = clock() - t;
    double time_taken = ((double) t) / CLOCKS_PER_SEC; // in seconds
    printf("%f\n", time_taken);

    return 0;
}
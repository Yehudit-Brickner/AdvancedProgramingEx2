#include <pthread.h>
#include <stdbool.h>
#include "codec.h"
#include "myArrayList.c"

#define CHUNK_SIZE 8

int argc1;
char **argv1;

arraylist array;

typedef struct {
    item *head;
    item *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} queue;

queue *create_queue() {
    queue *q = (queue *) malloc(sizeof(queue));
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
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

void *worker_function(void *arg) {
    while (!isEmpty(pool.task_queue)) {
        item *task = dequeue(pool.task_queue);
        encrypt(task->output, 34);
        if (toPrintIndex == task->index) {
            printf("%s: %d\n", task->output, task->index);
            toPrintIndex++;
        } else {
            add_at(&array, task, task->index);
            item *currItem;
            currItem = get(&array, toPrintIndex);
            while (currItem != NULL) {
                printf("%s: %d\n", currItem->output, currItem->index);
                toPrintIndex++;
                currItem = get(&array, toPrintIndex);
            }
        }
    }
    return NULL;
}
bool read_and_process(FILE *input) {
    char buffer[CHUNK_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, sizeof(char), CHUNK_SIZE, input)) > 0) {
        enqueue(pool.task_queue, buffer, currIndex);
        currIndex++;
    }

    return !ferror(input);
}

void *readerThreadFunction(void *arg) {
    printf("hiiiii\n");
    if (argc1 < 2 || strcmp(argv1[1], "-") == 0) {
        if (!read_and_process(stdin)) {
            printf("Error reading from stdin\n");
            return NULL;
        }
    } else {
        FILE *input = fopen(argv1[1], "r");
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
    return NULL;
}

int main(int argc, char **argv) {
    argc1 = argc;
    argv1 = argv;
    printf("started\n");
    initArrayList(&array);
    int num_threads = 4;
    pthread_t threads[num_threads];
    pool.task_queue = create_queue();

    pthread_t readerThread;
    pthread_create(&readerThread, NULL, readerThreadFunction, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_function, &pool);
    }

    while (!isEmpty(pool.task_queue)) {
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
    }

    return 0;
}

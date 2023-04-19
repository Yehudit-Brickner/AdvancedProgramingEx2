#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define MAX_STRING_SIZE 1024

typedef struct item {
    char *input;
    char *output;
    int index;
    struct item *next;
} item;

typedef struct {
    item *head;
    item *tail;
} queue;

void init(queue *q) {
    q->head = NULL;
    q->tail = NULL;
}

int is_empty(queue *q) {
    return q->head == NULL;
}

void enqueue(queue *q, const char *in, int index){
    item *new_item = malloc(sizeof(item));
    new_item->input = malloc(strlen(in) + 1);
    new_item->output = malloc(strlen(in) + 1);
    strcpy(new_item->input, in);
    strcpy(new_item->output, in); // later needs to be from the encrypt decrypt
    new_item->index=index;
    new_item->next = NULL;
    if (is_empty(q)) {
        q->head = new_item;
        q->tail = new_item;
    } else {
        q->tail->next = new_item;
        q->tail = new_item;
    }
}

char* dequeue(queue *q) {
    if (is_empty(q)) {
        printf("Error: Queue is empty\n");
        return NULL;
    }
    item *head = q->head;
    char* in = head->input;
    q->head = head->next;
    free(head->input);
    free(head->output);
    free(head);
    if (q->head == NULL) {
        q->tail = NULL;
    }
    return in;
}

void printhead(queue *q){
    if (is_empty(q)) {
        printf("Error: Queue is empty\n");
    }
    else{
        item *head = q->head;
        printf("[input: %s\n",head->input);
        printf("output: %s\n",head->output);
        printf("index: %d]\n",head->index);
    }
}


int main() {
    queue q;
    init(&q);
    printf("Queue is empty: %d\n", is_empty(&q));
    enqueue(&q, "Hello",1);
    printhead(&q);

    enqueue(&q, "World",2);
    printf("Queue is empty: %d\n", is_empty(&q));
    
    printhead(&q);
    
    char* str = dequeue(&q);
    printf("Dequeued: %s\n", str);
   
    printhead(&q);

    str = dequeue(&q);
    printf("Dequeued: %s\n", str);
 
    printf("Queue is empty: %d\n", is_empty(&q));
    return 0;
}
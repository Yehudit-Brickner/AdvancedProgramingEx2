#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 10

typedef struct item {
    char *data;
} item;

typedef struct {
    item *array;
    size_t size;
    size_t capacity;
} arraylist;

void init(arraylist *al) {
    al->size = 0;
    al->capacity = INITIAL_CAPACITY;
    al->array = malloc(al->capacity * sizeof(item));
}

void add(arraylist *al, const char *str) {
    if (al->size == al->capacity) {
        al->capacity *= 2;
        al->array = realloc(al->array, al->capacity * sizeof(item));
    }
    item *new_item = malloc(sizeof(item));
    new_item->data = malloc(strlen(str) + 1);
    strcpy(new_item->data, str);
    al->array[al->size++] = *new_item;
}

void add_at(arraylist *al, const item *currItem, size_t index) {
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
    al->array[index] = *currItem;
    al->size++;
}

item* get(arraylist *al, size_t index) {
    if (index >= al->size) {
        printf("Error: Index out of bounds\n");
        return NULL;
    }
    return &al->array[index];
}

void remove_item(arraylist *al, size_t index) {
    if (index >= al->size) {
        printf("Error: Index out of bounds\n");
        return;
    }
    item *item_to_remove = &al->array[index];
    free(item_to_remove->data);
    free(item_to_remove);
    memmove(&al->array[index], &al->array[index + 1], (al->size - index - 1) * sizeof(item));
    al->size--;
}

void destroy(arraylist *al) {
    for (size_t i = 0; i < al->size; i++) {
        free(al->array[i].data);
        free(&al->array[i]);
    }
    free(al->array);
}

int main() {
    arraylist al;
    init(&al);
    add(&al, "Hello");
    add(&al, "World");
    add_at(&al, "there", 1);
    item* i = get(&al, 2);
    printf("Got item: %s\n", i->data);
    remove_item(&al, 0);
    printf("Removed item 0\n");
    for (size_t j = 0; j < al.size; j++) {
        printf("%s ", al.array[j].data);
    }
    printf("\n");
    destroy(&al);
    return 0;
}
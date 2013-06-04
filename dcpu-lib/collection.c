#include "main.h"
#include "collection.h"

int initCollection(Collection* col, int initialCap) {
    col->cap = initialCap;
    col->used = 0;
    col->data = malloc(initialCap * sizeof(void*));
    if (col->data == NULL) {
        return 1;
    }
    return 0;
}

void freeCollection(Collection* col) {
    free(col->data);
}

int collectionAdd(Collection* col, void* element) {
    if (col->used == col->cap) {
        col->cap *= 2;
        void** tmp = realloc(col->data, col->cap * sizeof(void*));
        if (tmp == NULL) {
            col->cap /= 2;
            return 1;
        }
        col->data = tmp;
    }
    col->data[col->used++] = element;
    return 0;
}

bool collectionDel(Collection* col, void* element) {
    int i;
    for (i = 0; i < col->used; i++) {
        if (col->data[i] == element) {
            col->data[i] = col->data[--col->used];
            return true;
        }
    }
    return false;
}

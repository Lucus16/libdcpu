#ifndef COLLECTION_H_INCLUDED
#define COLLECTION_H_INCLUDED

typedef struct Collection {
    void** data;
    int used;
    int cap;
} Collection;

int initCollection(Collection* col, int initialCap);
void freeCollection(Collection* col);
int collectionAdd(Collection* col, void* element);
int collectionDel(Collection* col, void* element);

#endif // COLLECTION_H_INCLUDED

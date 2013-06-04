#ifndef COLLECTION_H_INCLUDED
#define COLLECTION_H_INCLUDED

typedef struct Collection {
    void** data;
    int used;
    int cap;
} Collection;

#endif // COLLECTION_H_INCLUDED

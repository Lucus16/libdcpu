#ifndef MANAGER_H_INCLUDED
#define MANAGER_H_INCLUDED

typedef struct Manager {
    DCPU* dcpus;
    int maxDcpus;
    Device* devices;
    int maxDevices;
    void (*onMemError)(Manager*);
} Manager;

#endif // MANAGER_H_INCLUDED

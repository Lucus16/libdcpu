#ifndef MANAGER_H_INCLUDED
#define MANAGER_H_INCLUDED

#include "m35fd.h"

typedef struct Manager Manager;

typedef struct Manager {
    Collection dcpus;
    Collection devices;
    Collection floppies;
} Manager;

Manager* newManager();
void freeManager(Manager* man);

DCPU* man_newDCPU(Manager* man);
int man_freeDCPU(Manager* man, DCPU* dcpu);

Floppy* man_newFloppy(Manager* man, const char* filename, bool writeProtected);
Floppy* man_loadFloppy(Manager* man, const char* filename, bool writeProtected);
int man_saveFloppy(Manager* man, Floppy* floppy);

Device* man_newDevice(Manager* man);
int man_freeDevice(Manager* man, Device* device);
int man_connectDevice(Manager* man, DCPU* dcpu, Device* device);
int man_disconnectDevice(Manager* man, Device* device);

Device* man_newClock(Manager* man);
Device* man_newM35FD(Manager* man);

#endif // MANAGER_H_INCLUDED

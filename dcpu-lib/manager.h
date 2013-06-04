#ifndef MANAGER_H_INCLUDED
#define MANAGER_H_INCLUDED

typedef struct Manager Manager;

typedef struct Manager {
    Collection dcpus;
    Collection devices;
} Manager;

Manager* newManager();
void freeManager(Manager* man);
DCPU* man_newDCPU(Manager* man);
int man_freeDCPU(Manager* man, DCPU* dcpu);
Device* man_newDevice(Manager* man);
int man_freeDevice(Manager* man, Device* device);
int man_connectDevice(Manager* man, DCPU* dcpu, Device* device);
int man_disconnectDevice(Manager* man, Device* device);

Device* man_newClock(Manager* man);

#endif // MANAGER_H_INCLUDED

#ifndef MANAGER_H_INCLUDED
#define MANAGER_H_INCLUDED

typedef struct Manager Manager;

typedef struct Manager {
    DCPU** dcpus;
    int dcpuCount;
    int dcpuCapacity;
    Device** devices;
    int deviceCount;
    int deviceCapacity;
} Manager;

Manager* newManager();
void freeManager(Manager* man);
DCPU* man_newDCPU(Manager* man);
int man_freeDCPU(Manager* man, DCPU* dcpu);
Device* man_newDevice(Manager* man);
int man_freeDevice(Manager* man, Device* device);

#endif // MANAGER_H_INCLUDED

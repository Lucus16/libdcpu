#include "dcpu.h"
#include "manager.h"
#include "genericclock.h"

Manager* newManager() {
    Manager* man = malloc(sizeof(Manager));
    initCollection(&man->dcpus, 4);
    initCollection(&man->devices, 16);
    return man;
}

void freeManager(Manager* man) {
    int i;
    for (i = 0; i < man->dcpus.used; i++) {
        destroyDCPU(man->dcpus.data[i]);
    }
    for (i = 0; i < man->devices.used; i++) {
        destroyDevice(man->devices.data[i]);
    }
    free(man->dcpus.data);
    free(man->devices.data);
    free(man);
}

DCPU* man_newDCPU(Manager* man) {
    DCPU* dcpu = newDCPU();
    if (dcpu == NULL) { return NULL; }
    if (collectionAdd(&man->dcpus, dcpu) != 0) {
        destroyDCPU(dcpu);
        return NULL;
    }
    return dcpu;
}

int man_freeDCPU(Manager* man, DCPU* dcpu) {
    int ret = collectionDel(&man->dcpus, dcpu);
    destroyDCPU(dcpu);
    return ret;
}

Device* man_newDevice(Manager* man) {
    Device* device = malloc(sizeof(Device));
    if (device == NULL) { return NULL; }
    if (collectionAdd(&man->devices, device) != 0) {
        destroyDevice(device);
        return NULL;
    }
    return device;
}

int man_freeDevice(Manager* man, Device* device) {
    int ret = collectionDel(&man->devices, device);
    man_disconnectDevice(man, device);
    destroyDevice(device);
    return ret;
}

int man_connectDevice(Manager* man, DCPU* dcpu, Device* device) {
    man_disconnectDevice(man, device);
    if (collectionAdd(&dcpu->devices, device) != 0) {
        return 1;
    }
    device->dcpu = dcpu;
    return 0;
}

int man_disconnectDevice(Manager* man, Device* device) {
    if (device->dcpu == NULL) { return 0; }
    int ret = collectionDel(&device->dcpu->devices, device);
    device->dcpu = NULL;
    return ret;
}

Device* man_newClock(Manager* man) {
    Device* device = man_newDevice(man);
    if (device == NULL) { return NULL; }
    initClock(device);
    return device;
}

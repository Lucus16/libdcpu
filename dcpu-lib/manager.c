#include "dcpu.h"
#include "manager.h"
#include "clock.h"

Manager* newManager() {
    Manager* man = malloc(sizeof(Manager));
    initCollection(&man->dcpus, 4);
    initCollection(&man->devices, 16);
    initCollection(&man->floppies, 4);
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
    for (i = 0; i < man->floppies.used; i++) {
        freeFloppy(man->floppies.data[i]);
    }
    free(man->dcpus.data);
    free(man->devices.data);
    free(man->floppies.data);
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

Floppy* man_newFloppy(Manager* man, char* filename, bool writeProtected) {
    Floppy* floppy = malloc(sizeof(Floppy));
    if (floppy == NULL) { return NULL; }
    if (collectionAdd(&man->floppies, floppy) != 0) {
        free(floppy);
        return NULL;
    }
    floppy->filename = filename;
    floppy->writeProtected = writeProtected;
    memset(floppy->data, 0, sizeof(floppy->data));
    return floppy;
}

Floppy* man_loadFloppy(Manager* man, char* filename, bool writeProtected) {
    Floppy* floppy = malloc(sizeof(Floppy));
    if (floppy == NULL) { return NULL; }
    if (flashFloppy(floppy, filename) != 0 || collectionAdd(&man->floppies, floppy) != 0) {
        free(floppy);
        return NULL;
    }
    floppy->filename = filename;
    floppy->writeProtected = writeProtected;
    return floppy;
}

int man_saveFloppy(Manager* man, Floppy* floppy) {
    if (floppy->filename == NULL) { return 2; }
    return dumpFloppy(floppy, floppy->filename);
}

Device* man_newDevice(Manager* man) {
    Device* device = malloc(sizeof(Device));
    if (device == NULL) { return NULL; }
    device->dcpu = NULL;
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
    device->dcpu = NULL;
    initClock(device);
    return device;
}

Device* man_newM35FD(Manager* man) {
    Device* device = man_newDevice(man);
    if (device == NULL) { return NULL; }
    device->dcpu = NULL;
    initM35FD(device);
    return device;
}

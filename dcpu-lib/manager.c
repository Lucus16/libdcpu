#include "dcpu.h"
#include "manager.h"

Manager* newManager() {
    Manager* man = malloc(sizeof(Manager));
    man->dcpus = malloc(4 * sizeof(void*));
    man->dcpuCount = 0;
    man->dcpuCapacity = 4;
    man->devices = malloc(16 * sizeof(void*));
    man->deviceCount = 0;
    man->deviceCapacity = 16;
    return man;
}

void freeManager(Manager* man) {
    int i;
    for (i = 0; i < man->dcpuCount; i++) {
        destroyDCPU(man->dcpus[i]);
    }
    for (i = 0; i < man->deviceCount; i++) {
        destroyDevice(man->devices[i]);
    }
    free(man->dcpus);
    free(man->devices);
    free(man);
}

DCPU* man_newDCPU(Manager* man) {
    if (man->dcpuCount == man->dcpuCapacity) {
        man->dcpuCapacity *= 2;
        DCPU** tmp = realloc(man->dcpus, man->dcpuCapacity * sizeof(void*));
        if (tmp == NULL) {
            man->deviceCapacity /= 2;
            return NULL;
        }
        man->dcpus = tmp;
    }
    DCPU* dcpu = newDCPU();
    if (dcpu == NULL) { return NULL; }
    man->dcpus[man->dcpuCount++] = dcpu;
    return dcpu;
}

int man_freeDCPU(Manager* man, DCPU* dcpu) {
    int i;
    for (i = 0; i < man->dcpuCount; i++) {
        if (man->dcpus[i] == dcpu) {
            destroyDCPU(dcpu);
            man->dcpus[i] = man->dcpus[--man->dcpuCount];
            man->dcpus[man->dcpuCount] = NULL;
            return 0;
        }
    }
    return 1;
}

Device* man_newDevice(Manager* man) {
    if (man->deviceCount == man->deviceCapacity) {
        man->deviceCapacity *= 2;
        Device** tmp = realloc(man->devices, man->deviceCapacity * sizeof(void*));
        if (tmp == NULL) {
            man->deviceCapacity /= 2;
            return NULL;
        }
        man->devices = tmp;
    }
    Device* device = malloc(sizeof(Device));
    if (device == NULL) { return NULL; }
    man->devices[man->deviceCount++] = device;
    return device;
}

int man_freeDevice(Manager* man, Device* device) {
    int i;
    for (i = 0; i < man->deviceCount; i++) {
        if (man->devices[i] == device) {
            destroyDevice(device);
            man->devices[i] = man->devices[--man->deviceCount];
            man->devices[man->deviceCount] = NULL;
            return 0;
        }
    }
    return 1;
}

void man_connectDevice(Manager* man, DCPU* dcpu, Device* device) {
    if (dcpu->deviceCount == dcpu->deviceCapacity) {
        dcpu->deviceCapacity *= 2;
        Device** tmp = realloc(dcpu->devices, dcpu->deviceCapacity * sizeof(void*));
        if (tmp == NULL) {
            dcpu->deviceCapacity /= 2;
            return NULL;
        }
        dcpu->devices = tmp;
    }
    if (device->dcpu != NULL) {
        man_disconnectDevice(man, device->dcpu, device);
    }
    dcpu->devices[dcpu->deviceCount++] = device;
    device->dcpu = dcpu;
}

int man_disconnectDevice(Manager* man, DCPU* dcpu, Device* device) {
    int i;
    for (i = 0; i < dcpu->deviceCount; i++) {
        if (dcpu->devices[i] == device) {
            dcpu->devices[i] = dcpu->devices[--dcpu->deviceCount];
            dcpu->devices[dcpu->deviceCount] = NULL;
            device->dcpu = NULL;
            return 0;
        }
    }
    return 1;
}

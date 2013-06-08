#include "m35fd.h"

int initM35FD(Device* dev) {
    dev->data = malloc(sizeof(M35FD));
    if (dev->data == NULL) { return 1; }
    dev->super.ID = 0x4fd524c5;
    dev->super.version = 0x000b;
    dev->super.manufacturer = 0x1eb37e91;
    dev->interruptHandler = m35fdHandler;
    dev->reset = m35fdReset;
    dev->destroyData = NULL;
    m35fdReset(dev);
    return 0;
}

void m35fdHandler(Device* dev) {
    M35FD* m35fd = dev->data;
    switch (dev->dcpu->regA) {
        case 0: //POLL
            dev->dcpu->regB = m35fd->state;
            dev->dcpu->regC = m35fd->error;
            m35fd->error = ERROR_NONE;
            break;
        case 1: //SET INTERRUPT
            m35fd->interruptMessage = dev->dcpu->regX;
            break;
        case 3: //WRITE SECTOR
            if (m35fd->state == STATE_READY_WP) {
                m35fd->error = ERROR_PROTECTED;
                dev->dcpu->regB = 0;
                return;
            }
        case 2: //READ SECTOR
            if (m35fd->state == STATE_NO_MEDIA) {
                m35fd->error = ERROR_NO_MEDIA;
                dev->dcpu->regB = 0;
                return;
            }
            if (m35fd->state == STATE_BUSY) {
                m35fd->error = ERROR_BUSY;
                dev->dcpu->regB = 0;
                return;
            }
            m35fd->targetSector = dev->dcpu->regX;
            m35fd->targetMemory = dev->dcpu->regY;
            m35fd->currentOperation = dev->dcpu->regB;
            m35fd->currentEvent = addEvent(dev->dcpu->eventchain, abs(m35fd->currentTrack - m35fd->targetSector / 18) * 240, m35fdSwitchTrack, dev);
            m35fd->state = STATE_BUSY;
            dev->dcpu->regB = 1;
            break;
    }
}

void m35fdReset(Device* device) {
    M35FD* m35fd = device->data;
    if (device->dcpu != NULL && m35fd->currentEvent != NULL) {
        removeEvent(device->dcpu->eventchain, m35fd->currentEvent);
    }
    m35fd->currentEvent = NULL;
    m35fd->currentOperation = 0;
    m35fd->currentTrack = 0;
    m35fd->interruptMessage = 0;
    m35fd->targetMemory = 0;
    m35fd->targetSector = 0;
    m35fd->state = m35fd->floppy == NULL ? STATE_NO_MEDIA : (m35fd->floppy->writeProtected ? STATE_READY_WP : STATE_READY);
}

void m35fdSwitchTrack(void* data) {
    Device* device = data;
    M35FD* m35fd = device->data;
    m35fd->currentTrack = m35fd->targetSector / 18;
    m35fd->currentEvent = addEvent(device->dcpu->eventchain, 1668, m35fdOperate, device);
}

void m35fdOperate(void* data) {
    Device* device = data;
    M35FD* m35fd = device->data;
    word* floppymem = m35fd->floppy->data + 512 * m35fd->targetSector;
    word* dcpumem = device->dcpu->mem + m35fd->targetMemory;
    if (m35fd->currentOperation == 2) {
        memcpy(dcpumem, floppymem, 512 * sizeof(word));
    } else {
        memcpy(floppymem, dcpumem, 512 * sizeof(word));
    }
    m35fd->state = m35fd->floppy->writeProtected ? STATE_READY_WP : STATE_READY;
    if (m35fd->interruptMessage != 0) {
        addInterrupt(device->dcpu, m35fd->interruptMessage);
    }
    m35fd->currentEvent = NULL;
}

int m35fdInsertFloppy(Device* device, Floppy* floppy) {
    M35FD* m35fd = device->data;
    if (m35fd->floppy != NULL) { return 1; }
    m35fd->floppy = floppy;
    m35fd->state = floppy->writeProtected ? STATE_READY_WP : STATE_READY;
    if (m35fd->interruptMessage != 0) {
        addInterrupt(device->dcpu, m35fd->interruptMessage);
    }
    return 0;
}

int m35fdEjectFloppy(Device* device) {
    M35FD* m35fd = device->data;
    if (m35fd->floppy == NULL) { return 1; }
    if (m35fd->state == STATE_BUSY) {
        m35fd->error = ERROR_EJECT;
    }
    m35fd->floppy = NULL;
    m35fd->state = STATE_NO_MEDIA;
    if (m35fd->interruptMessage != 0) {
        addInterrupt(device->dcpu, m35fd->interruptMessage);
    }
    return 0;
}

Floppy* newFloppy(bool writeProtected) {
    Floppy* floppy = malloc(sizeof(Floppy));
    floppy->filename = NULL;
    floppy->writeProtected = writeProtected;
    memset(floppy->data, 0, sizeof(floppy->data));
    return floppy;
}

void freeFloppy(Floppy* floppy) {
    free(floppy);
}

int dumpFloppy(Floppy* floppy, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        return 1;
    }
    int i;
    for (i = 0; i < 737280; i++) {
        fputc(floppy->data[i] >> 8, file);
        fputc(floppy->data[i] & 0xff, file);
    }
    fclose(file);
    return 0;
}

int flashFloppy(Floppy* floppy, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return 1;
    }
    int i, a, b;
    for (i = 0; i < 737280; i++) {
        a = fgetc(file);
        if (a == EOF) { break; }
        b = fgetc(file);
        if (b == EOF) { break; }
        floppy->data[i] = a << 8 | b;
    }
    fclose(file);
    return 0;
}

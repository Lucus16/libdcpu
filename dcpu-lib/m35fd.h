#ifndef M35FD_H_INCLUDED
#define M35FD_H_INCLUDED

#include "dcpu.h"

typedef struct Floppy {
    const char* filename;
    bool writeProtected;
    word data[737280];
} Floppy;

typedef struct M35FD {
    word interruptMessage;
    enum {STATE_NO_MEDIA=0, STATE_READY=1, STATE_READY_WP=2, STATE_BUSY=3} state;
    enum {ERROR_NONE=0, ERROR_BUSY=1, ERROR_NO_MEDIA=2, ERROR_PROTECTED=3, ERROR_EJECT=4, ERROR_BAD_SECTOR=5, ERROR_BROKEN=0xffff} error;
    Floppy* floppy;
    Event* currentEvent;
    int currentOperation;
    int currentTrack;
    int targetSector;
    int targetMemory;
} M35FD;

int initM35FD(Device* dev);
void m35fdHandler(Device* dev);
void m35fdReset(Device* dev);
void m35fdSwitchTrack(void* data);
void m35fdOperate(void* data);
int m35fdInsertFloppy(Device* device, Floppy* floppy);
Floppy* newFloppy(bool writeProtected);
void freeFloppy(Floppy* floppy);
int dumpFloppy(Floppy* floppy, const char* filename);
int saveFloppy(Floppy* floppy);
int flashFloppy(Floppy* floppy, const char* filename);
Floppy* loadFloppy(const char* filename, bool writeProtected);

#endif // M35FD_H_INCLUDED

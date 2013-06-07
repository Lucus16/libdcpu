#ifndef DCPU_H_INCLUDED
#define DCPU_H_INCLUDED

#include "main.h"
#include "event.h"
#include "collection.h"

#define regA reg[0]
#define regB reg[1]
#define regC reg[2]
#define regX reg[3]
#define regY reg[4]
#define regZ reg[5]
#define regI reg[6]
#define regJ reg[7]
#define regSP reg[8]
#define regPC reg[9]
#define regEX reg[10]
#define regIA reg[11]

#define push(dcpu, val) dcpu->mem[--dcpu->reg[8]] = val
#define pop(dcpu) dcpu->mem[dcpu->reg[8]++]

typedef uint16_t word;
typedef union {uint16_t u; int16_t s;} wordu;
typedef int64_t cycles_t;
typedef struct {uint16_t ID; uint8_t version; uint16_t manufacturer;} Super;

typedef struct DCPU DCPU;
typedef struct Event Event;
typedef struct Device Device;

typedef struct DCPU {
    word mem[65536];
    word reg[12]; //A, B, C, X, Y, Z, I, J, SP, PC, EX, IA
    bool skipping;
    bool queuing;
    bool onfire;
    bool running;
    int hertz;
    cycles_t cycleno;
    word interrupts[256];
    uint8_t firstInterrupt;
    int interruptCount;
    void (*onfirefn)(DCPU*);
    void (*oninvalid)(DCPU*);
    void (*onbreak)(DCPU*);
    Collection devices;
    Event* eventchain;
} DCPU;

typedef struct Device {
    DCPU* dcpu;
    Super super;
    void* data;
    void (*interruptHandler)(Device*);
    void (*reset)(Device*);
    void (*destroyData)(Device*);
} Device;

DCPU* newDCPU();
void destroyDCPU(DCPU* dcpu);
void rebootDCPU(DCPU* dcpu, bool clearmem);
int flashDCPU(DCPU* dcpu, char* filename);
int dumpMemory(DCPU* dcpu, char* filename);
int docycles(DCPU* dcpu, cycles_t cyclestodo);
void addInterrupt(DCPU* dcpu, word value);
void destroyDevice(Device* device);

void setB(DCPU* dcpu, int argb, word value);

#endif // DCPU_H_INCLUDED

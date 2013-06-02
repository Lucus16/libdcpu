#ifndef DCPU_H_INCLUDED
#define DCPU_H_INCLUDED

#include "main.h"
#include "event.h"

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
    int deviceCount;
    Device* devices;
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
int flashDCPU(DCPU* dcpu, const char* filename);
int docycles(DCPU* dcpu, cycles_t cyclestodo);
void addInterrupt(DCPU* dcpu, word value);
void destroyDevice(Device* device);

word getA(DCPU* dcpu, word instruction);
word getB(DCPU* dcpu, word instruction);
void setA(DCPU* dcpu, word instruction, word value);
void setB(DCPU* dcpu, word instruction, word value);

void SET(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void ADD(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void SUB(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void MUL(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void MLI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void DIV(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void DVI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void MOD(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void MDI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void AND(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void BOR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void XOR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void SHR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void ASR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void SHL(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFB(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFC(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFE(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFN(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFG(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFA(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFL(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void IFU(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void ADX(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void SBX(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void STI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void STD(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);
void INV(DCPU* dcpu, word instruction, wordu aValue, wordu bValue);

void JSR(DCPU* dcpu, word instruction, wordu aValue);
void INT(DCPU* dcpu, word instruction, wordu aValue);
void IAG(DCPU* dcpu, word instruction, wordu aValue);
void IAS(DCPU* dcpu, word instruction, wordu aValue);
void RFI(DCPU* dcpu, word instruction, wordu aValue);
void IAQ(DCPU* dcpu, word instruction, wordu aValue);
void HWN(DCPU* dcpu, word instruction, wordu aValue);
void HWQ(DCPU* dcpu, word instruction, wordu aValue);
void HWI(DCPU* dcpu, word instruction, wordu aValue);
void AIV(DCPU* dcpu, word instruction, wordu aValue);

#endif // DCPU_H_INCLUDED

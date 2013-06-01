#include "dcpu.h"

DCPU* newDCPU() {
    DCPU* dcpu = malloc(sizeof(DCPU));
    if (dcpu == NULL) {
        return NULL;
    }
    dcpu->onfirefn = NULL;
    dcpu->oninvalid = NULL;
    dcpu->onbreak = NULL;
    dcpu->devices = NULL;
    dcpu->nextevent = NULL;
    dcpu->deviceCount = 0;
    dcpu->hertz = 100000;
    rebootDCPU(dcpu, true);
    return dcpu;
}

void destroyDCPU(DCPU* dcpu) {
    //free all events
    Event* ne = dcpu->nextevent;
    Event* del = NULL;
    while (ne) {
        del = ne;
        ne = ne->nextevent;
        free(del);
    }
    free(dcpu);
    //NOTE: Does not free devices.
}

void destroyDevice(Device* device) {
    if (device->destroyData != NULL) {
        device->destroyData(device);
    } else {
        free(device->data);
    }
    free(device);
}

void rebootDCPU(DCPU* dcpu, bool clearmem) {
    dcpu->skipping = false;
    dcpu->queuing = false;
    dcpu->onfire = false;
    dcpu->running = false;
    dcpu->cycleno = 0;
    dcpu->firstInterrupt = 0;
    dcpu->interruptCount = 0;
    if (clearmem) { memset(dcpu->mem, 0, sizeof(dcpu->mem)); }
    memset(dcpu->interrupts, 0, 512);
    memset(dcpu->reg, 0, 24);
}

int flashDCPU(DCPU* dcpu, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return 1;
    }
    int i, a, b;
    for (i = 0; i < 65536; i++) {
        a = fgetc(file);
        if (a == EOF) { break; }
        b = fgetc(file);
        if (b == EOF) { break; }
        dcpu->mem[i] = a << 8 | b;
    }
    memset(dcpu->mem + i, 0, 131072 - i);
    fclose(file);
    return 0;
}

//Returns the number of cycles executed
int docycles(DCPU* dcpu, int cyclestodo) {
    static void (*basicFunctions[32])(DCPU*, word, wordu, wordu) = {
        INV, SET, ADD, SUB, MUL, MLI, DIV, DVI,
        MOD, MDI, AND, BOR, XOR, SHR, ASR, SHL,
        IFB, IFC, IFE, IFN, IFG, IFA, IFL, IFU,
        INV, INV, ADX, SBX, INV, INV, STI, STD
    };
    static void (*advancedFunctions[32])(DCPU*, word, wordu) = {
        AIV, JSR, AIV, AIV, AIV, AIV, AIV, AIV,
        INT, IAG, IAS, RFI, IAQ, AIV, AIV, AIV,
        HWN, HWQ, HWI, AIV, AIV, AIV, AIV, AIV,
        AIV, AIV, AIV, AIV, AIV, AIV, AIV, AIV
    };

    if (!dcpu->running && cyclestodo != -1) { return 0; }
    if (cyclestodo == -1) { cyclestodo = 1; }
    int targetcycles = dcpu->cycleno + cyclestodo;
    while ((dcpu->cycleno < targetcycles) && dcpu->running) {
        if (dcpu->onfire && dcpu->onfirefn) {
            dcpu->onfirefn(dcpu);
        }
        if (!dcpu->queuing && dcpu->interruptCount && !dcpu->skipping) {
            if (!dcpu->regIA) {
                push(dcpu, dcpu->regPC);
                push(dcpu, dcpu->regA);
                dcpu->regPC = dcpu->regIA;
                dcpu->regA = dcpu->interrupts[dcpu->firstInterrupt];
            }
            dcpu->firstInterrupt++;
            dcpu->interruptCount--;
        }
        Event* ne = dcpu->nextevent;
        while (ne) {
            if (ne->time <= dcpu->cycleno) {
                ne->ontrigger(dcpu, ne->data);
                dcpu->nextevent = ne->nextevent;
                free(ne);
            } else {
                break;
            }
            ne = dcpu->nextevent;
        }
        word instruction = dcpu->mem[dcpu->regPC++];
        int opcode = instruction & 0x1f;
        wordu aValue = (wordu)getA(dcpu, instruction);
        wordu bValue;
        if (opcode != 0) {
            bValue = (wordu)getB(dcpu, instruction);
        }
        if (dcpu->skipping) {
            dcpu->cycleno++;
            dcpu->skipping = (opcode > 0xf && opcode < 0x18);
        } else {
            if (opcode != 0) {
                (*basicFunctions[instruction & 0x1f])(dcpu, instruction, aValue, bValue);
            } else {
                (*advancedFunctions[(instruction >> 5) & 0x1f])(dcpu, instruction, aValue);
            }
        }
    }
    return dcpu->cycleno - (targetcycles - cyclestodo);
}

void addInterrupt(DCPU* dcpu, word value) {
    dcpu->interruptCount++;
    dcpu->interrupts[(dcpu->firstInterrupt + ++dcpu->interruptCount) & 0xff] = value;
    if (dcpu->interruptCount > 256) {
        dcpu->onfire = true;
    }
}

Event* addEvent(DCPU* dcpu, int time, void (*ontrigger)(DCPU*, void*), void* data) {
    Event* event = malloc(sizeof(event));
    if (event == NULL) {
        return NULL; //error: out of memory
    }
    event->ontrigger = ontrigger;
    event->data = data;
    event->time = time;
    time += dcpu->cycleno;
    Event* ne = dcpu->nextevent;
    if (ne == NULL) {
        event->nextevent = NULL;
        dcpu->nextevent = event;
    } else {
        while (true) {
            if (ne->nextevent != NULL) {
                if (ne->nextevent->time >= time) {
                    event->nextevent = ne->nextevent;
                    ne->nextevent = event;
                    break;
                }
            } else {
                event->nextevent = NULL;
                ne->nextevent = event;
                break;
            }
            ne = ne->nextevent;
        }
    }
    return event;
}

int removeEvent(DCPU* dcpu, Event* event) {
    Event* ne = dcpu->nextevent;
    if (ne == NULL) {
        return 1;
    } else if (ne == event) {
        dcpu->nextevent = ne->nextevent;
        free(event);
        return 0;
    }
    while (ne != NULL) {
        if (ne->nextevent == event) {
            ne->nextevent = event->nextevent;
            free(event);
            return 0;
        }
    }
    return 1;
}

void SET(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, aValue.u);
    dcpu->cycleno += 1;
}

void ADD(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u + aValue.u);
    dcpu->regEX = bValue.u + aValue.u > 0xffff ? 1 : 0;
    dcpu->cycleno += 2;
}

void SUB(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u - aValue.u);
    dcpu->regEX = bValue.u - aValue.u < 0 ? -1 : 0;
    dcpu->cycleno += 2;
}

void MUL(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u * aValue.u);
    dcpu->regEX = (bValue.u * aValue.u) >> 16;
    dcpu->cycleno += 2;
}

void MLI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.s * aValue.s);
    dcpu->regEX = (bValue.s * aValue.s) >> 16;
    dcpu->cycleno += 2;
}

void DIV(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    if (aValue.u == 0) {
        setB(dcpu, instruction, 0);
        dcpu->regEX = 0;
    } else {
        setB(dcpu, instruction, bValue.u / aValue.u);
        dcpu->regEX = (bValue.u << 16) / aValue.u;
    }
    dcpu->cycleno += 3;
}

void DVI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    if (aValue.s == 0) {
        setB(dcpu, instruction, 0);
        dcpu->regEX = 0;
    } else {
        setB(dcpu, instruction, bValue.s / aValue.s);
        dcpu->regEX = (bValue.s << 16) / aValue.s;
    }
    dcpu->cycleno += 3;
}

void MOD(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    if (aValue.u == 0) {
        setB(dcpu, instruction, 0);
    } else {
        setB(dcpu, instruction, bValue.u % aValue.u);
    }
    dcpu->cycleno += 3;
}

void MDI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    if (aValue.s == 0) {
        setB(dcpu, instruction, 0);
    } else {
        setB(dcpu, instruction, bValue.s % aValue.s);
    }
    dcpu->cycleno += 3;
}

void AND(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u & aValue.u);
    dcpu->cycleno += 1;
}

void BOR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u | aValue.u);
    dcpu->cycleno += 1;
}

void XOR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u ^ aValue.u);
    dcpu->cycleno += 1;
}

void SHR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u >> aValue.u);
    dcpu->cycleno += 1;
}

void ASR(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.s >> aValue.u);
    dcpu->cycleno += 1;
}

void SHL(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, bValue.u << aValue.u);
    dcpu->cycleno += 1;
}

void IFB(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = !(bValue.u & aValue.u);
    dcpu->cycleno += 2;
}

void IFC(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = bValue.u & aValue.u;
    dcpu->cycleno += 2;
}

void IFE(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = !(bValue.u == aValue.u);
    dcpu->cycleno += 2;
}

void IFN(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = bValue.u == aValue.u;
    dcpu->cycleno += 2;
}

void IFG(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = bValue.u <= aValue.u;
    dcpu->cycleno += 2;
}

void IFA(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = bValue.s <= aValue.s;
    dcpu->cycleno += 2;
}

void IFL(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = bValue.u >= aValue.u;
    dcpu->cycleno += 2;
}

void IFU(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->skipping = bValue.s >= aValue.s;
    dcpu->cycleno += 2;
}

void ADX(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    int tmp = bValue.u + aValue.u + dcpu->regEX;
    setB(dcpu, instruction, tmp);
    dcpu->regEX = tmp > 0xffff ? 1 : 0;
    dcpu->cycleno += 3;
}

void SBX(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    int tmp = bValue.u - aValue.u + dcpu->regEX;
    setB(dcpu, instruction, tmp);
    dcpu->regEX = tmp > 0xffff ? 1 : tmp < 0 ? -1 : 0;
    dcpu->cycleno += 3;
}

void STI(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, aValue.u);
    dcpu->regI++;
    dcpu->regJ++;
    dcpu->cycleno += 2;
}

void STD(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    setB(dcpu, instruction, aValue.u);
    dcpu->regI--;
    dcpu->regJ--;
    dcpu->cycleno += 2;
}

void INV(DCPU* dcpu, word instruction, wordu aValue, wordu bValue) {
    dcpu->cycleno += 1;
    if (dcpu->oninvalid) {
        dcpu->oninvalid(dcpu);
    }
}

void JSR(DCPU* dcpu, word instruction, wordu aValue) {
    push(dcpu, dcpu->regPC);
    dcpu->regPC = aValue.u;
    dcpu->cycleno += 3;
}

void INT(DCPU* dcpu, word instruction, wordu aValue) {
    addInterrupt(dcpu, aValue.u);
    dcpu->cycleno += 4;
}

void IAG(DCPU* dcpu, word instruction, wordu aValue) {
    setA(dcpu, instruction, dcpu->regIA);
    dcpu->cycleno += 1;
}

void IAS(DCPU* dcpu, word instruction, wordu aValue) {
    dcpu->regIA = aValue.u;
    dcpu->cycleno += 1;
}

void RFI(DCPU* dcpu, word instruction, wordu aValue) {
    dcpu->queuing = false;
    dcpu->regA = pop(dcpu);
    dcpu->regPC = pop(dcpu);
    dcpu->cycleno += 3;
}

void IAQ(DCPU* dcpu, word instruction, wordu aValue) {
    dcpu->queuing = aValue.u;
    dcpu->cycleno += 2;
}

void HWN(DCPU* dcpu, word instruction, wordu aValue) {
    setA(dcpu, instruction, dcpu->deviceCount);
    dcpu->cycleno += 2;
}

void HWQ(DCPU* dcpu, word instruction, wordu aValue) {
    if (aValue.u < dcpu->deviceCount) {
        Super* super = &dcpu->devices[aValue.u].super;
        dcpu->regA = super->ID & 0xffff;
        dcpu->regB = super->ID >> 16;
        dcpu->regC = super->version;
        dcpu->regX = super->manufacturer & 0xffff;
        dcpu->regY = super->manufacturer >> 16;
    }
    dcpu->cycleno += 4;
}

void HWI(DCPU* dcpu, word instruction, wordu aValue) {
    dcpu->devices[aValue.u].interruptHandler(&dcpu->devices[aValue.u]);
    dcpu->cycleno += 4;
}

void AIV(DCPU* dcpu, word instruction, wordu aValue) {
    dcpu->cycleno += 1;
    if (dcpu->oninvalid) {
        dcpu->oninvalid(dcpu);
    }
}

word getA(DCPU* dcpu, word instruction) {
    int val = instruction >> 10;
    switch (val >> 3) {
        case 0:
            return dcpu->reg[val & 0x7];
        case 1:
            return dcpu->mem[dcpu->reg[val & 0x7]];
        case 2:
            dcpu->cycleno++;
            return dcpu->mem[(dcpu->reg[val & 0x7] + dcpu->mem[dcpu->regPC++]) & 0xffff];
        case 3:
            switch (val & 0x7) {
                case 0: //[SP++]
                    return dcpu->mem[dcpu->regSP++];
                case 1: //[SP]
                    return dcpu->mem[dcpu->regSP];
                case 2: //[SP + [PC++]]
                    dcpu->cycleno++;
                    return dcpu->mem[(dcpu->regSP + dcpu->mem[dcpu->regPC++]) & 0xffff];
                case 3:
                case 4:
                case 5:
                    return dcpu->reg[val - 19];
                case 6:
                    dcpu->cycleno++;
                    return dcpu->mem[dcpu->mem[dcpu->regPC++]];
                case 7:
                    dcpu->cycleno++;
                    return dcpu->mem[dcpu->regPC++];
            }
        default:
            return val - 33;
    }
}

word getB(DCPU* dcpu, word instruction) {
    int val = (instruction >> 5) & 0x1f;
    switch (val >> 3) {
        case 0:
            return dcpu->reg[val & 0x7];
        case 1:
            return dcpu->mem[dcpu->reg[val & 0x7]];
        case 2:
            dcpu->cycleno++;
            return dcpu->mem[(dcpu->reg[val & 0x7] + dcpu->mem[dcpu->regPC++]) & 0xffff];
        case 3:
            switch (val & 0x7) {
                case 0: //[--SP]
                    return dcpu->mem[--dcpu->regSP];
                case 1: //[SP]
                    return dcpu->mem[dcpu->regSP];
                case 2: //[SP + [PC++]]
                    dcpu->cycleno++;
                    return dcpu->mem[(dcpu->regSP + dcpu->mem[dcpu->regPC++]) & 0xffff];
                case 3:
                case 4:
                case 5:
                    return dcpu->reg[val - 19];
                case 6:
                    dcpu->cycleno++;
                    return dcpu->mem[dcpu->mem[dcpu->regPC++]];
                case 7:
                    dcpu->cycleno++;
                    return dcpu->mem[dcpu->regPC++];
            }
        default:
            return 0;
    }
}

void setA(DCPU* dcpu, word instruction, word value) {
    int val = (instruction >> 5) & 0x1f;
    switch (val >> 3) {
        case 0:
            dcpu->reg[val & 0x7] = value;
            return;
        case 1:
            dcpu->mem[dcpu->reg[val & 0x7]] = value;
            return;
        case 2:
            dcpu->mem[(dcpu->reg[val & 0x7] + dcpu->mem[(dcpu->regPC - 1) & 0xffff]) & 0xffff] = value;
            return;
        case 3:
            switch (val & 0x7) {
                case 0:
                    dcpu->mem[dcpu->regSP - 1] = value;
                    return;
                case 1:
                    dcpu->mem[dcpu->regSP] = value;
                    return;
                case 2:
                    dcpu->mem[(dcpu->regSP + dcpu->mem[(dcpu->regPC - 1) & 0xffff]) & 0xffff] = value;
                    return;
                case 3:
                case 4:
                case 5:
                    dcpu->reg[val - 19] = value;
                    return;
                case 6:
                    dcpu->mem[dcpu->mem[(dcpu->regPC - 1) & 0xffff]] = value;
                    return;
            }
    }
}

void setB(DCPU* dcpu, word instruction, word value) {
    int val = (instruction >> 5) & 0x1f;
    switch (val >> 3) {
        case 0:
            dcpu->reg[val & 0x7] = value;
            return;
        case 1:
            dcpu->mem[dcpu->reg[val & 0x7]] = value;
            return;
        case 2:
            dcpu->mem[(dcpu->reg[val & 0x7] + dcpu->mem[(dcpu->regPC - 1) & 0xffff]) & 0xffff] = value;
            return;
        case 3:
            switch (val & 0x7) {
                case 0:
                case 1:
                    dcpu->mem[dcpu->regSP] = value;
                    return;
                case 2:
                    dcpu->mem[(dcpu->regSP + dcpu->mem[(dcpu->regPC - 1) & 0xffff]) & 0xffff] = value;
                    return;
                case 3:
                case 4:
                case 5:
                    dcpu->reg[val - 19] = value;
                    return;
                case 6:
                    dcpu->mem[dcpu->mem[(dcpu->regPC - 1) & 0xffff]] = value;
                    return;
            }
    }
}

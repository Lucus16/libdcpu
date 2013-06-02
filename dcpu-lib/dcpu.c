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
    dcpu->eventchain = newChain();
    dcpu->deviceCount = 0;
    dcpu->hertz = 100000;
    rebootDCPU(dcpu, true);
    return dcpu;
}

void destroyDCPU(DCPU* dcpu) {
    destroyChain(dcpu->eventchain);
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
    memset(dcpu->interrupts, 0, sizeof(dcpu->interrupts));
    memset(dcpu->reg, 0, sizeof(dcpu->reg));
    destroyChain(dcpu->eventchain);
    dcpu->eventchain = newChain();
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
int docycles(DCPU* dcpu, cycles_t cyclestodo) {
    static void (*basicFunctions[32])(DCPU*, word, wordu, wordu) = {
        INV, SET, ADD, SUB, MUL, MLI, DIV, DVI,
        MOD, MDI, AND, BOR, XOR, SHR, ASR, SHL,
        IFB, IFC, IFE, IFN, IFG, IFA, IFL, IFU,
        INV, INV, ADX, SBX, INV, INV, STI, STD
    };
    static int valueLengths[64] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,0,0,1,0,0,0,1,1,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };

    if (!dcpu->running && cyclestodo != -1) { return 0; }
    if (cyclestodo == -1) { cyclestodo = 1; }
    cycles_t targetcycles = dcpu->cycleno + cyclestodo;
    Event* event = dcpu->eventchain->nextevent;
    wordu aValue, bValue;
    word instruction;
    int opcode, arga, argb;
    while ((dcpu->cycleno < targetcycles) && dcpu->running) {
        if (dcpu->onfire && dcpu->onfirefn) {
            dcpu->onfirefn(dcpu);
        }
        if (dcpu->interruptCount != 0 && !dcpu->queuing && !dcpu->skipping) {
            if (!dcpu->regIA) {
                push(dcpu, dcpu->regPC);
                push(dcpu, dcpu->regA);
                dcpu->regPC = dcpu->regIA;
                dcpu->regA = dcpu->interrupts[dcpu->firstInterrupt];
            }
            dcpu->firstInterrupt++;
            dcpu->interruptCount--;
        }
        while (event != NULL && event->time <= dcpu->cycleno) {
            event->ontrigger(event->data);
            Event* del = event;
            event = event->nextevent;
            free(del);
            dcpu->eventchain->nextevent = event;
        }
        instruction = dcpu->mem[dcpu->regPC++];
        opcode = instruction & 0x1f;
        arga = instruction >> 10;
        argb = (instruction >> 5) & 0x1f;
        if (dcpu->skipping) {
            dcpu->cycleno++;
            dcpu->regPC += valueLengths[instruction >> 10] + valueLengths[(instruction >> 5) & 0x1f];
            dcpu->skipping = (opcode > 0xf && opcode < 0x18);
        } else {
            switch (arga) {
                case 0:
                    aValue.u = dcpu->regA;
                    break;
                case 1:
                    aValue.u = dcpu->regB;
                    break;
                case 2:
                    aValue.u = dcpu->regC;
                    break;
                case 3:
                    aValue.u = dcpu->regX;
                    break;
                case 4:
                    aValue.u = dcpu->regY;
                    break;
                case 5:
                    aValue.u = dcpu->regZ;
                    break;
                case 6:
                    aValue.u = dcpu->regI;
                    break;
                case 7:
                    aValue.u = dcpu->regJ;
                    break;
                case 8:
                    aValue.u = dcpu->mem[dcpu->regA];
                    break;
                case 9:
                    aValue.u = dcpu->mem[dcpu->regB];
                    break;
                case 10:
                    aValue.u = dcpu->mem[dcpu->regC];
                    break;
                case 11:
                    aValue.u = dcpu->mem[dcpu->regX];
                    break;
                case 12:
                    aValue.u = dcpu->mem[dcpu->regY];
                    break;
                case 13:
                    aValue.u = dcpu->mem[dcpu->regZ];
                    break;
                case 14:
                    aValue.u = dcpu->mem[dcpu->regI];
                    break;
                case 15:
                    aValue.u = dcpu->mem[dcpu->regJ];
                    break;
                case 16:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regA + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 17:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regB + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 18:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regC + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 19:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regX + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 20:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regY + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 21:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regZ + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 22:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regI + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 23:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regJ + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 24:
                    aValue.u = dcpu->mem[dcpu->regSP++];
                    break;
                case 25:
                    aValue.u = dcpu->mem[dcpu->regSP];
                    break;
                case 26:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[(dcpu->regSP + dcpu->mem[dcpu->regPC++]) & 0xffff];
                    break;
                case 27:
                    aValue.u = dcpu->regSP;
                    break;
                case 28:
                    aValue.u = dcpu->regPC;
                    break;
                case 29:
                    aValue.u = dcpu->regEX;
                    break;
                case 30:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[dcpu->mem[dcpu->regPC++]];
                    break;
                case 31:
                    dcpu->cycleno++;
                    aValue.u = dcpu->mem[dcpu->regPC++];
                    break;
                default:
                    aValue.u = arga - 33;
                    break;
            }

            if (opcode != 0) {
                switch (argb) {
                    case 0:
                        bValue.u = dcpu->regA;
                        break;
                    case 1:
                        bValue.u = dcpu->regB;
                        break;
                    case 2:
                        bValue.u = dcpu->regC;
                        break;
                    case 3:
                        bValue.u = dcpu->regX;
                        break;
                    case 4:
                        bValue.u = dcpu->regY;
                        break;
                    case 5:
                        bValue.u = dcpu->regZ;
                        break;
                    case 6:
                        bValue.u = dcpu->regI;
                        break;
                    case 7:
                        bValue.u = dcpu->regJ;
                        break;
                    case 8:
                        bValue.u = dcpu->mem[dcpu->regA];
                        break;
                    case 9:
                        bValue.u = dcpu->mem[dcpu->regB];
                        break;
                    case 10:
                        bValue.u = dcpu->mem[dcpu->regC];
                        break;
                    case 11:
                        bValue.u = dcpu->mem[dcpu->regX];
                        break;
                    case 12:
                        bValue.u = dcpu->mem[dcpu->regY];
                        break;
                    case 13:
                        bValue.u = dcpu->mem[dcpu->regZ];
                        break;
                    case 14:
                        bValue.u = dcpu->mem[dcpu->regI];
                        break;
                    case 15:
                        bValue.u = dcpu->mem[dcpu->regJ];
                        break;
                    case 16:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regA + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 17:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regB + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 18:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regC + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 19:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regX + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 20:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regY + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 21:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regZ + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 22:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regI + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 23:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regJ + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 24:
                        bValue.u = dcpu->mem[--dcpu->regSP];
                        break;
                    case 25:
                        bValue.u = dcpu->mem[dcpu->regSP];
                        break;
                    case 26:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[(dcpu->regSP + dcpu->mem[dcpu->regPC++]) & 0xffff];
                        break;
                    case 27:
                        bValue.u = dcpu->regSP;
                        break;
                    case 28:
                        bValue.u = dcpu->regPC;
                        break;
                    case 29:
                        bValue.u = dcpu->regEX;
                        break;
                    case 30:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[dcpu->mem[dcpu->regPC++]];
                        break;
                    case 31:
                        dcpu->cycleno++;
                        bValue.u = dcpu->mem[dcpu->regPC++];
                        break;
                }

                (*basicFunctions[opcode])(dcpu, instruction, aValue, bValue); //TODO: Expand this
            } else {
                switch (argb) {
                    case 1: //JSR
                        push(dcpu, dcpu->regPC);
                        dcpu->regPC = aValue.u;
                        dcpu->cycleno += 3;
                        break;
                    case 8: //INT
                        addInterrupt(dcpu, aValue.u);
                        dcpu->cycleno += 4;
                        break;
                    case 9: //IAG
                        setA(dcpu, instruction, dcpu->regIA);
                        dcpu->cycleno += 1;
                        break;
                    case 10: //IAS
                        dcpu->regIA = aValue.u;
                        dcpu->cycleno += 1;
                        break;
                    case 11: //RFI
                        dcpu->queuing = false;
                        dcpu->regA = pop(dcpu);
                        dcpu->regPC = pop(dcpu);
                        dcpu->cycleno += 3;
                        break;
                    case 12: //IAQ
                        dcpu->queuing = aValue.u;
                        dcpu->cycleno += 3;
                        break;
                    case 16: //HWN
                        setA(dcpu, instruction, dcpu->deviceCount);
                        dcpu->cycleno += 2;
                        break;
                    case 17: //HWQ
                        if (aValue.u < dcpu->deviceCount) {
                            Super* super = &dcpu->devices[aValue.u].super;
                            dcpu->regA = super->ID & 0xffff;
                            dcpu->regB = super->ID >> 16;
                            dcpu->regC = super->version;
                            dcpu->regX = super->manufacturer & 0xffff;
                            dcpu->regY = super->manufacturer >> 16;
                        }
                        dcpu->cycleno += 4;
                        break;
                    case 18: //HWI
                        dcpu->devices[aValue.u].interruptHandler(dcpu->devices + aValue.u);
                        dcpu->cycleno += 4;
                        break;
                    default:
                        dcpu->cycleno += 1;
                        if (dcpu->oninvalid != NULL) {
                            dcpu->oninvalid(dcpu);
                        }
                        break;
                }
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

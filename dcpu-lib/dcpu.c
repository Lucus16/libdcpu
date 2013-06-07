#include "dcpu.h"

DCPU* newDCPU() {
    DCPU* dcpu = malloc(sizeof(DCPU));
    if (dcpu == NULL) { return NULL; }
    dcpu->onfirefn = NULL;
    dcpu->oninvalid = NULL;
    dcpu->onbreak = NULL;
    initCollection(&dcpu->devices, 16);
    dcpu->eventchain = newChain();
    dcpu->hertz = 100000;
    rebootDCPU(dcpu, true);
    return dcpu;
}

void destroyDCPU(DCPU* dcpu) {
    destroyChain(dcpu->eventchain);
    free(dcpu->devices.data);
    free(dcpu);
    //NOTE: Does not free the devices themselves.
}

void destroyDevice(Device* device) {
    if (device->destroyData != NULL) {
        device->destroyData(device);
    } else if (device->data != NULL) {
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

int flashDCPU(DCPU* dcpu, char* filename) {
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

int dumpMemory(DCPU* dcpu, char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        return 1;
    }
    int i;
    for (i = 0; i < 65536; i++) {
        fputc(dcpu->mem[i] >> 8, file);
        fputc(dcpu->mem[i] & 0xff, file);
    }
    fclose(file);
    return 0;
}

//Returns the number of cycles executed
int docycles(DCPU* dcpu, cycles_t cyclestodo) {
    static int valueLengths[64] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,0,0,1,0,0,0,1,1,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    static int validBasic[32] = {
        0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,0,0,1,1,0,0,1,1
    };
    static int validAdvanced[32] = {
        0,1,0,0,0,0,0,0,1,1,1,1,1,0,0,0,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0
    };

    if (!dcpu->running && cyclestodo != -1) { return 0; }
    if (cyclestodo == -1) { cyclestodo = 1; }
    cycles_t targetcycles = dcpu->cycleno + cyclestodo;
    Event* event = dcpu->eventchain->nextevent;
    Event* del;
    Super* super;
    wordu aValue, bValue;
    word instruction;
    int opcode, arga, argb, tmp;
    while ((dcpu->cycleno < targetcycles) && dcpu->running) {
        if (dcpu->onfire && dcpu->onfirefn) {
            dcpu->onfirefn(dcpu);
        }
        if (dcpu->interruptCount != 0 && !dcpu->queuing && !dcpu->skipping) {
            if (dcpu->regIA != 0) {
                push(dcpu, dcpu->regPC);
                push(dcpu, dcpu->regA);
                dcpu->regPC = dcpu->regIA;
                dcpu->regA = dcpu->interrupts[dcpu->firstInterrupt];
                dcpu->queuing = true;
            }
            dcpu->firstInterrupt++;
            dcpu->interruptCount--;
        }
        event = dcpu->eventchain->nextevent;
        while (event != NULL && event->time <= dcpu->cycleno) {
            event->ontrigger(event->data);
            del = event;
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
            dcpu->regPC += valueLengths[arga] + valueLengths[argb];
            dcpu->skipping = (opcode > 0xf && opcode < 0x18);
        } else if (validBasic[opcode] || (opcode == 0 && validAdvanced[argb])) {
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

                switch (opcode) {
                    case 1: //SET
                        setB(dcpu, argb, aValue.u);
                        dcpu->cycleno += 1;
                        break;
                    case 2: //ADD
                        setB(dcpu, argb, bValue.u + aValue.u);
                        dcpu->regEX = bValue.u + aValue.u > 0xffff ? 1 : 0;
                        dcpu->cycleno += 2;
                        break;
                    case 3: //SUB
                        setB(dcpu, argb, bValue.u - aValue.u);
                        dcpu->regEX = bValue.u - aValue.u < 0 ? -1 : 0;
                        dcpu->cycleno += 2;
                        break;
                    case 4: //MUL
                        setB(dcpu, argb, bValue.u * aValue.u);
                        dcpu->regEX = (bValue.u * aValue.u) >> 16;
                        dcpu->cycleno += 2;
                        break;
                    case 5: //MLI
                        setB(dcpu, argb, bValue.s * aValue.s);
                        dcpu->regEX = (bValue.s * aValue.s) >> 16;
                        dcpu->cycleno += 2;
                        break;
                    case 6: //DIV
                        if (aValue.u == 0) {
                            setB(dcpu, argb, 0);
                            dcpu->regEX = 0;
                        } else {
                            setB(dcpu, argb, bValue.u / aValue.u);
                            dcpu->regEX = (bValue.u << 16) / aValue.u;
                        }
                        dcpu->cycleno += 3;
                        break;
                    case 7: //DVI
                        if (aValue.s == 0) {
                            setB(dcpu, argb, 0);
                            dcpu->regEX = 0;
                        } else {
                            setB(dcpu, argb, bValue.s / aValue.s);
                            dcpu->regEX = (bValue.s << 16) / aValue.s;
                        }
                        dcpu->cycleno += 3;
                        break;
                    case 8: //MOD
                        if (aValue.u == 0) {
                            setB(dcpu, argb, 0);
                        } else {
                            setB(dcpu, argb, bValue.u % aValue.u);
                        }
                        dcpu->cycleno += 3;
                        break;
                    case 9: //MDI
                        if (aValue.s == 0) {
                            setB(dcpu, argb, 0);
                        } else {
                            setB(dcpu, argb, bValue.s % aValue.s);
                        }
                        dcpu->cycleno += 3;
                        break;
                    case 10: //AND
                        setB(dcpu, argb, bValue.u & aValue.u);
                        dcpu->cycleno += 1;
                        break;
                    case 11: //BOR
                        setB(dcpu, argb, bValue.u | aValue.u);
                        dcpu->cycleno += 1;
                        break;
                    case 12: //XOR
                        setB(dcpu, argb, bValue.u ^ aValue.u);
                        dcpu->cycleno += 1;
                        break;
                    case 13: //SHR
                        setB(dcpu, argb, bValue.u >> aValue.u);
                        dcpu->regEX = ((int)bValue.u << 16) >> aValue.u;
                        dcpu->cycleno += 1;
                        break;
                    case 14: //ASR
                        setB(dcpu, argb, bValue.s >> aValue.u);
                        dcpu->regEX = ((int)bValue.s << 16) >> aValue.u;
                        dcpu->cycleno += 1;
                        break;
                    case 15: //SHL
                        setB(dcpu, argb, bValue.u << aValue.u);
                        dcpu->regEX = ((int)bValue.u << aValue.u) >> 16;
                        dcpu->cycleno += 1;
                        break;
                    case 16: //IFB
                        dcpu->skipping = (bValue.u & aValue.u) == 0;
                        dcpu->cycleno += 2;
                        break;
                    case 17: //IFC
                        dcpu->skipping = (bValue.u & aValue.u) != 0;
                        dcpu->cycleno += 2;
                        break;
                    case 18: //IFE
                        dcpu->skipping = bValue.u != aValue.u;
                        dcpu->cycleno += 2;
                        break;
                    case 19: //IFN
                        dcpu->skipping = bValue.u == aValue.u;
                        dcpu->cycleno += 2;
                        break;
                    case 20: //IFG
                        dcpu->skipping = bValue.u <= aValue.u;
                        dcpu->cycleno += 2;
                        break;
                    case 21: //IFA
                        dcpu->skipping = bValue.s <= aValue.s;
                        dcpu->cycleno += 2;
                        break;
                    case 22: //IFL
                        dcpu->skipping = bValue.u >= aValue.u;
                        dcpu->cycleno += 2;
                        break;
                    case 23: //IFU
                        dcpu->skipping = bValue.s >= aValue.s;
                        dcpu->cycleno += 2;
                        break;
                    case 26: //ADX
                        tmp = bValue.u + aValue.u + dcpu->regEX;
                        setB(dcpu, argb, tmp);
                        dcpu->regEX = tmp > 0xffff ? 1 : 0;
                        dcpu->cycleno += 3;
                        break;
                    case 27: //SBX
                        tmp = bValue.u - aValue.u + dcpu->regEX;
                        setB(dcpu, argb, tmp);
                        dcpu->regEX = tmp > 0xffff ? 1 : (tmp < 0 ? -1 : 0);
                        dcpu->cycleno += 3;
                        break;
                    case 30: //STI
                        setB(dcpu, argb, aValue.u);
                        dcpu->regI++;
                        dcpu->regJ++;
                        dcpu->cycleno += 2;
                        break;
                    case 31: //STD
                        setB(dcpu, argb, aValue.u);
                        dcpu->regI--;
                        dcpu->regJ--;
                        dcpu->cycleno += 2;
                        break;
                }

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
                        setA(dcpu, arga, dcpu->regIA);
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
                        setA(dcpu, arga, dcpu->devices.used);
                        dcpu->cycleno += 2;
                        break;
                    case 17: //HWQ
                        if (aValue.u < dcpu->devices.used) {
                            super = &((Device*)dcpu->devices.data[aValue.u])->super;
                            dcpu->regA = super->ID & 0xffff;
                            dcpu->regB = super->ID >> 16;
                            dcpu->regC = super->version;
                            dcpu->regX = super->manufacturer & 0xffff;
                            dcpu->regY = super->manufacturer >> 16;
                        }
                        dcpu->cycleno += 4;
                        break;
                    case 18: //HWI
                        ((Device*)dcpu->devices.data[aValue.u])->interruptHandler(*(dcpu->devices.data + aValue.u));
                        dcpu->cycleno += 4;
                        break;
                }

            }
        } else {
            dcpu->cycleno++;
            if (dcpu->oninvalid != NULL) {
                dcpu->oninvalid(dcpu);
            }
        }
    }
    return dcpu->cycleno - (targetcycles - cyclestodo);
}

void addInterrupt(DCPU* dcpu, word value) {
    dcpu->interrupts[(dcpu->firstInterrupt + ++dcpu->interruptCount) & 0xff] = value;
    if (dcpu->interruptCount > 256) {
        dcpu->onfire = true;
    }
}

void setA(DCPU* dcpu, int arga, word value) {
    switch (arga >> 3) {
        case 0:
            dcpu->reg[arga & 0x7] = value;
            return;
        case 1:
            dcpu->mem[dcpu->reg[arga & 0x7]] = value;
            return;
        case 2:
            dcpu->mem[(dcpu->reg[arga & 0x7] + dcpu->mem[(dcpu->regPC - 1) & 0xffff]) & 0xffff] = value;
            return;
        case 3:
            switch (arga & 0x7) {
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
                    dcpu->regSP = value;
                    return;
                case 4:
                    dcpu->regPC = value;
                    return;
                case 5:
                    dcpu->regEX = value;
                    return;
                case 6:
                    dcpu->mem[dcpu->mem[(dcpu->regPC - 1) & 0xffff]] = value;
                    return;
            }
    }
}

void setB(DCPU* dcpu, int argb, word value) {
    switch (argb >> 3) {
        case 0:
            dcpu->reg[argb & 0x7] = value;
            return;
        case 1:
            dcpu->mem[dcpu->reg[argb & 0x7]] = value;
            return;
        case 2:
            dcpu->mem[(dcpu->reg[argb & 0x7] + dcpu->mem[(dcpu->regPC - 1) & 0xffff]) & 0xffff] = value;
            return;
        case 3:
            switch (argb & 0x7) {
                case 0:
                case 1:
                    dcpu->mem[dcpu->regSP] = value;
                    return;
                case 2:
                    dcpu->mem[(dcpu->regSP + dcpu->mem[(dcpu->regPC - 1) & 0xffff]) & 0xffff] = value;
                    return;
                case 3:
                    dcpu->regSP = value;
                    return;
                case 4:
                    dcpu->regPC = value;
                    return;
                case 5:
                    dcpu->regEX = value;
                    return;
                case 6:
                    dcpu->mem[dcpu->mem[(dcpu->regPC - 1) & 0xffff]] = value;
                    return;
            }
    }
}

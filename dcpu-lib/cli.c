#include "main.h"
#include "cli.h"

CLIData* newCLI() {
    CLIData* clidata = malloc(sizeof(CLIData));
    clidata->man = newManager();
    clidata->nextDcpuID = 0;
    return clidata;
}

void freeCLI(CLIData* clidata) {
    freeManager(clidata->man);
    free(clidata);
}

void printStatus(DCPU* dcpu) {
    printf("Name: %-37.37s A: 0x%.4x [A]: 0x%.4x [SP]: 0x%.4x\n",
           dcpu->name, dcpu->regA, dcpu->mem[dcpu->regA], dcpu->mem[dcpu->regSP]);
    printf("Queuing:  %c  Interrupts: %-4.1i Devices: %-4.1i B: 0x%.4x [B]: 0x%.4x [+1]: 0x%.4x\n",
           (dcpu->queuing ? 'Y' : 'N'), dcpu->interruptCount, dcpu->devices.used,
           dcpu->regB, dcpu->mem[dcpu->regB], dcpu->mem[(dcpu->regSP + 1) & 0xffff]);
    printf("On fire:  %c  Cycleno: %-21.1li C: 0x%.4x [C]: 0x%.4x [+2]: 0x%.4x\n",
           (dcpu->onfire ? 'Y' : 'N'), dcpu->cycleno,
           dcpu->regC, dcpu->mem[dcpu->regC], dcpu->mem[(dcpu->regSP + 2) & 0xffff]);
    printf("Skipping: %c  Hertz: %-23.1i X: 0x%.4x [X]: 0x%.4x [+3]: 0x%.4x\n",
           (dcpu->skipping ? 'Y' : 'N'), dcpu->hertz,
           dcpu->regX, dcpu->mem[dcpu->regX], dcpu->mem[(dcpu->regSP + 3) & 0xffff]);
    printf("SP: 0x%.4x PC: 0x%.4x I: 0x%.4x [I]: 0x%.4x Y: 0x%.4x [Y]: 0x%.4x [+4]: 0x%.4x\n",
           dcpu->regSP, dcpu->regPC, dcpu->regI, dcpu->mem[dcpu->regI],
           dcpu->regY, dcpu->mem[dcpu->regY], dcpu->mem[(dcpu->regSP + 4) & 0xffff]);
    printf("EX: 0x%.4x IA: 0x%.4x J: 0x%.4x [J]: 0x%.4x Z: 0x%.4x [Z]: 0x%.4x [+5]: 0x%.4x\n",
           dcpu->regEX, dcpu->regIA, dcpu->regJ, dcpu->mem[dcpu->regJ],
           dcpu->regZ, dcpu->mem[dcpu->regZ], dcpu->mem[(dcpu->regSP + 5) & 0xffff]);
}

void runCommand(CLIData* clidata, char input[82]) {
    char tmpstr[82];
    char tmpstr2[82];
    int tmpint;
    Manager* man = clidata->man;
    if (sscanf(input, "new dcpu %s", tmpstr) == 1) {
        DCPU* tmpdcpu = man_newDCPU(man);
        snprintf(tmpdcpu->name, 40, "%s", tmpstr);
        printf("New DCPU created: %s\n", tmpdcpu->name);
        clidata->dcpu = tmpdcpu;
        printf("Main DCPU is now: %s\n", clidata->dcpu->name);
        return;
    }
    if (strcmp(input, "new dcpu") == 0) {
        DCPU* tmpdcpu = man_newDCPU(man);
        snprintf(tmpdcpu->name, 40, "dcpu%i", clidata->nextDcpuID++);
        printf("New DCPU created: %s\n", tmpdcpu->name);
        clidata->dcpu = tmpdcpu;
        printf("Main DCPU is now: %s\n", clidata->dcpu->name);
        return;
    }
    if (sscanf(input, "select dcpu %s", tmpstr) == 1) {
        int i;
        for (i = 0; i < man->dcpus.used; i++) {
            if (strncmp(tmpstr, ((DCPU*)man->dcpus.data[i])->name, 40) == 0) {
                clidata->dcpu = man->dcpus.data[i];
                printf("Main DCPU is now: %s\n", clidata->dcpu->name);
                return;
            }
        }
        printf("Could not find DCPU called: %s\n", tmpstr);
        return;
    }
    if (sscanf(input, "del dcpu %s", tmpstr) == 1) {
        int i;
        for (i = 0; i < man->dcpus.used; i++) {
            if (strcmp(tmpstr, ((DCPU*)man->dcpus.data[i])->name) == 0) {
                man_freeDCPU(man, man->dcpus.data[i]);
                printf("DCPU deleted: %s\n", tmpstr);
                return;
            }
        }
        printf("Could not find DCPU called: %s\n", tmpstr);
        return;
    }
    if (strcmp(input, "del dcpu") == 0) {
        if (clidata->dcpu != NULL) {
            printf("DCPU deleted: %s\n", clidata->dcpu->name);
            man_freeDCPU(man, clidata->dcpu);
            clidata->dcpu = NULL;
            return;
        } else {
            printf("There is no main dcpu to be deleted.\n");
            return;
        }
    }
    if (sscanf(input, "docycles %i", &tmpint) == 1) {
        if (clidata->dcpu == NULL) {
            printf("There is no main dcpu to do cycles on.\n");
            return;
        }
        clidata->dcpu->running = true;
        tmpint = docycles(clidata->dcpu, tmpint);
        clidata->dcpu->running = false;
        printf("%i cycles were executed on main dcpu.\n", tmpint);
        return;
    }
    if (strcmp(input, "step") == 0) {
        tmpint = docycles(clidata->dcpu, -1);
        printf("%i cycles were executed on main dcpu.\n", tmpint);
        return;
    }
    if (strcmp(input, "status") == 0) {
        if (clidata->dcpu == NULL) {
            printf("There is no main dcpu to display the status of.\n");
        } else {
            printStatus(clidata->dcpu);
        }
        return;
    }
    if (sscanf(input, "load floppy %s as %s", tmpstr, tmpstr2) == 2) {
        Floppy* tmpfloppy = man_loadFloppy(man, tmpstr, false);
        if (tmpfloppy == NULL) {
            printf("Could not load floppy.\n");
        } else {
            strncpy(tmpfloppy->name, tmpstr2, 40);
            printf("Floppy loaded.\n");
        }
        return;
    }
    if (sscanf(input, "load read-only floppy %s as %s", tmpstr, tmpstr2) == 2) {
        Floppy* tmpfloppy = man_loadFloppy(man, tmpstr, true);
        if (tmpfloppy == NULL) {
            printf("Could not load floppy.\n");
        } else {
            strncpy(tmpfloppy->name, tmpstr2, 40);
            printf("Floppy loaded.\n");
        }
        return;
    }
    if (sscanf(input, "insert floppy %s in %s", tmpstr, tmpstr2) == 2) {
        int i;
        for (i = 0; i < man->devices.used; i++) {
            if (strcmp(tmpstr2, ((Device*)man->devices.data[i])->name) == 0) {
                Device* m35fd = man->devices.data[i];
                for (i = 0; i < man->floppies.used; i++) {
                    if (strcmp(tmpstr, ((Floppy*)man->floppies.data[i])->name) == 0) {
                        m35fdInsertFloppy(m35fd, man->floppies.data[i]);
                        printf("Floppy inserted.\n");
                        return;
                    }
                }
                printf("Could not find Floppy called: %s\n", tmpstr);
                return;
            }
        }
        printf("Could not find M35FD called: %s\n", tmpstr2);
        return;
    }
    if (sscanf(input, "add clock %s", tmpstr) == 1) {
        Device* tmpclock = man_newClock(man);
        if (tmpclock == NULL) {
            printf("Out of memory - clock not created.\n");
            return;
        }
        strncpy(tmpclock->name, tmpstr, 40);
        if (clidata->dcpu != NULL) {
            man_connectDevice(man, clidata->dcpu, tmpclock);
            printf("New clock created and connected.\n");
        } else {
            printf("New clock created.\n");
        }
        return;
    }
    if (sscanf(input, "add m35fd %s", tmpstr) == 1) {
        Device* tmpm35fd = man_newM35FD(man);
        if (tmpm35fd == NULL) {
            printf("Out of memory - M35FD not created.\n");
            return;
        }
        strncpy(tmpm35fd->name, tmpstr, 40);
        if (clidata->dcpu != NULL) {
            man_connectDevice(man, clidata->dcpu, tmpm35fd);
            printf("New M35FD created and connected.\n");
        } else {
            printf("New M35FD created.\n");
        }
        return;
    }
    if (sscanf(input, "flash dcpu %s", tmpstr) == 1) {
        if (flashDCPU(clidata->dcpu, tmpstr) == 0) {
            printf("Successfully loaded.\n");
        } else {
            printf("Failed to open file.\n");
        }
        return;
    }
    if (sscanf(input, "flash floppy %s", tmpstr) == 1) {
        if (flashFloppy(clidata->floppy, tmpstr) == 0) {
            printf("Successfully loaded.\n");
        } else {
            printf("Failed to open file.\n");
        }
        return;
    }
    if (sscanf(input, "load %s", tmpstr) == 1) {
        FILE* file = fopen(tmpstr, "r");
        if (file == NULL) {
            printf("Failed to open file.\n");
            return;
        }
        while (!feof(file)) {
            fgets(input, 82, file);
            if (feof(file)) { break; }
            int l = strlen(input);
            if (input[l - 1] == '\n') { input[--l] = '\0'; }
            if (l == 0) { continue; }
            printf("file>%s\n", input);
            runCommand(clidata, input);
        }
        return;
    }
    printf("Command not understood.\n");
}

void cliMainLoop() {
    CLIData* clidata = newCLI();
    char input[82];
    while (true) {
        printf("user>");
        fgets(input, 82, stdin);
        int l = strlen(input);
        if (input[l - 1] == '\n') { input[--l] = '\0'; }
        if (l == 0) { continue; }
        if (strncmp(input, "quit", 4) == 0) {
            break;
        } else if (strcmp(input, "q") == 0) {
            break;
        } else {
            runCommand(clidata, input);
        }
    }
    freeCLI(clidata);
}


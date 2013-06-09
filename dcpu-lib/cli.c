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

void runCommand(CLIData* clidata, char input[82]) {
    char tmpstr[82];
    char tmpstr2[82];
    Manager* man = clidata->man;
    DCPU* dcpu = NULL;
    Floppy* floppy = NULL;
    if (sscanf(input, "new dcpu %s", tmpstr) == 1) {
        DCPU* tmpdcpu = man_newDCPU(man);
        snprintf(tmpdcpu->name, 40, "%s", tmpstr);
        printf("New DCPU created: %s\n", tmpdcpu->name);
        dcpu = tmpdcpu;
        printf("Main DCPU is now: %s\n", dcpu->name);
        return;
    }
    if (strncmp(input, "new dcpu", 8) == 0) {
        DCPU* tmpdcpu = man_newDCPU(man);
        snprintf(tmpdcpu->name, 40, "dcpu%i", clidata->nextDcpuID++);
        printf("New DCPU created: %s\n", tmpdcpu->name);
        dcpu = tmpdcpu;
        printf("Main DCPU is now: %s\n", dcpu->name);
        return;
    }
    if (sscanf(input, "load floppy %s as %s", tmpstr, tmpstr2) == 1) {
        Floppy* tmpfloppy = man_loadFloppy(man, tmpstr, true);
        if (tmpfloppy == NULL) {
            printf("Could not load floppy.\n");
        } else {
            printf("Floppy loaded.\n");
        }
        return;
    }
    if (strncmp(input, "add clock", 9) == 0) {
        Device* tmpclock = man_newClock(man);
        if (dcpu != NULL) {
            man_connectDevice(man, dcpu, tmpclock);
            printf("New clock created and connected.\n");
        } else {
            printf("New clock created.\n");
        }
        return;
    }
    if (strncmp(input, "add m35fd", 9) == 0) {
        Device* tmpm35fd = man_newM35FD(man);
        if (dcpu != NULL) {
            man_connectDevice(man, dcpu, tmpm35fd);
            printf("New M35FD created and connected.\n");
        } else {
            printf("New M35FD created.\n");
        }
        return;
    }
    if (sscanf(input, "flash dcpu %s", tmpstr) == 1) {
        if (flashDCPU(dcpu, tmpstr) == 0) {
            printf("Successfully loaded.\n");
        } else {
            printf("Failed to open file.\n");
        }
        return;
    }
    if (sscanf(input, "flash floppy %s", tmpstr) == 1) {
        if (flashFloppy(floppy, tmpstr) == 0) {
            printf("Successfully loaded.\n");
        } else {
            printf("Failed to open file.\n");
        }
        return;
    }
    if (sscanf(input, "script %s", tmpstr) == 1) {
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


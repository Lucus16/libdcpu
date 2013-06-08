#include "main.h"
#include "cli.h"

CLIData* newCLI() {
    CLIData* clidata = malloc(sizeof(CLIData));
    clidata->man = newManager();
    clidata->nextDcpuID = 0;
    return clidata;
}

void runCommand(CLIData* clidata, char input[82]) {
    char tmpstr[82];
    if (strncmp(input, "new ", 4) == 0) {
        if (strncmp(input + 4, "dcpu", 4) == 0) {
            if (strncmp(input + 8, " ", 1) == 0) {
                DCPU* dcpu = man_newDCPU(clidata->man);
                strncpy(dcpu->name, input + 9, 72);
                dcpu->name[81] = '\0';
            } else {
                DCPU* dcpu = man_newDCPU(clidata->man);
                sprintf(tmpstr, "dcpu%i", clidata->nextDcpuID++);
                dcpu->name = malloc(strlen(tmpstr) + 1);
                strncpy(dcpu->name, tmpstr, 81);
                dcpu->name[81] = '\0';
            }
        }
    }
}

void cliMainLoop(CLIData* clidata) {
    char input[82];
    while (true) {
        fgets(input, 82, stdin);
        if (strncmp(input, "quit", 4) == 0) {
            break;
        } else {
            runCommand(clidata, input);
        }
    }
}


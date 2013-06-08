#include "main.h"
#include "cli.h"

CLIData* newCLI() {
    CLIData* clidata = malloc(sizeof(CLIData));
    return clidata;
}

void runCommand(char input[82]) {
    if (strncmp(input, "new ", 4) == 0) {
        if (strncmp(input + 4, "dcpu", 4) == 0) {
            if (strncmp(input + 8, " ", 1) == 0) {
                //Create new named dcpu
            } else {
                //Create new numbered dcpu
            }
        }
    }
}

void cliMainLoop() {
    char input[82];
    while (true) {
        fgets(input, 82, stdin);
        if (strncmp(input, "quit", 4) == 0) {
            break;
        } else {
            runCommand(input);
        }
    }
}

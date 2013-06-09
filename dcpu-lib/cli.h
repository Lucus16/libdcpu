#ifndef CLI_H_INCLUDED
#define CLI_H_INCLUDED

#include "manager.h"

typedef struct CLIData {
    Manager* man;
    int nextDcpuID;
} CLIData;

CLIData* newCLI();
void freeCLI(CLIData* clidata);
void runCommand(CLIData* clidata, char input[82]);
void cliMainLoop();

#endif // CLI_H_INCLUDED

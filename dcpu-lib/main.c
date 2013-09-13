#include "main.h"
#include "cli.h"

int main(int argc, char* argv[]) {
    CLIData* clidata = newCLI();
    int i;
    for (i = 1; i < argc; i++) {
        runCommand(clidata, argv[i]);
    }
    char input[82];
    while (true) {
        printf("  user> ");
        fgets(input, 82, stdin);
        int l = strlen(input);
        if (input[l - 1] == '\n') { input[--l] = '\0'; }
        if (l == 0) { continue; }
        if (strncmp(input, "quit", 4) == 0) {
            break;
        } else if (strncmp(input, "q", 1) == 0) {
            break;
        } else if (strncmp(input, "exit", 4) == 0) {
            break;
        } else {
            runCommand(clidata, input);
        }
    }
    freeCLI(clidata);
    return 0;
}

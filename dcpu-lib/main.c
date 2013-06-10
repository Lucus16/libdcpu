#include "main.h"
#include "dcpu.h"
#include "manager.h"
#include <time.h>

int main(int argc, char *argv[]) {
    Manager* man = newManager();
    DCPU* dcpu = man_newDCPU(man);
    if (argc == 2) {
        int result = flashDCPU(dcpu, argv[1]);
        if (result != 0) {
            printf("Failed to open file.\n");
            return 1;
        }
        printf("File loaded successfully.\n");
    } else {
        printf("No file selected, running on empty memory.\n");
    }
    dcpu->running = true;
    int start = clock();
    docycles(dcpu, 1000000000);
    int finish = clock();
    printf("Running one billion cycles took %lims.\n", (finish - start) / CLOCKS_PER_SEC * 1000);
    freeManager(man);
    return 0;
}

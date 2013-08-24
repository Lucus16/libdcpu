#include "main.h"
#include "dcpu.h"
#include "manager.h"
#include "pthread.h"
#include <time.h>

const int THREADNUM = 20;
const int PERTHREAD = 250;

typedef struct ThreadData {
    Manager* man;
    int n;
} ThreadData;

void* rundcpus(ThreadData* td) {
    int i, j;
    for (j = 0; j < 1000; j++) {
        for (i = td->n * PERTHREAD; i < (td->n + 1) * PERTHREAD; i++) {
            docycles(((DCPU*)td->man->dcpus.data[i]), 1000);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    long int start_time, finish_time;
    struct timespec gettime_now;

    Manager* man = newManager();
    int i;
    for (i = 0; i < THREADNUM * PERTHREAD; i++) {
        DCPU* dcpu = man_newDCPU(man);
        dcpu->running = true;
    }
    DCPU* dcpu0 = man->dcpus.data[0];
    int result = flashDCPU(dcpu0, argv[1]);
    if (result != 0) {
        printf("Failed to load file.\n");
        exit(1);
    }
    for (i = 1; i < THREADNUM * PERTHREAD; i++) {
        memcpy(((DCPU*)(man->dcpus.data[i]))->mem, dcpu0->mem, 65536 * sizeof(word));
    }
    //programs loaded.
    printf("Loading finished.\n");
    pthread_t threads[THREADNUM];
    ThreadData tds[THREADNUM];
    clock_gettime(CLOCK_REALTIME, &gettime_now);
    start_time = gettime_now.tv_nsec + 1000000000 * gettime_now.tv_sec;
    for (i = 0; i < THREADNUM; i++) {
        //make thread
        tds[i].man = man;
        tds[i].n = i;
        pthread_create(&threads[i], NULL, rundcpus, &tds[i]);
    }
    for (i = 0; i < THREADNUM; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &gettime_now);
    finish_time = gettime_now.tv_nsec + 1000000000 * gettime_now.tv_sec;
    printf("Running took %fms.\n", (float)(finish_time - start_time) / 1000000.0);
    //finish up
    freeManager(man);
    return 0;
}

int main0(int argc, char *argv[]) {
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
    printf("Running one billion cycles took %lims.\n", (finish - start) * 1000 / CLOCKS_PER_SEC);
    freeManager(man);
    return 0;
}

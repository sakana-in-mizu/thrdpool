#include "thrdpool.h"

#include <stdatomic.h>
#include <stdio.h>

atomic_int sum = 0;

#define N 100000

void compute(void *arg) {
    for (int i = 0; i < N; ++i) {
        ++sum;
    }
}

void terminate(void *arg) {
    thrdpool_t *pool = arg;

    thrdpool_free(pool, NULL);
    printf("%d\n", sum);
    printf("done.\n");
}

int main() {
    thrdpool_t *pool = thrdpool_new(8, 0);
    struct thrdpool_task compute_task = {compute, NULL},
                         terminate_task = {terminate, pool};

    for (int i = 0; i < 5; ++i) {
        thrdpool_schedule(pool, &compute_task);
    }
    thrdpool_schedule(pool, &terminate_task);

    getchar();
    return 0;
}
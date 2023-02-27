#ifndef __THRDPOOL_H__
#define __THRDPOOL_H__

#include <stddef.h>

typedef struct __thrdpool thrdpool_t;

struct thrdpool_task {
    void (*routine)(void *);
    void *context;
};

thrdpool_t *thrdpool_new(size_t nthreads, size_t stacksize);
int thrdpool_schedule(thrdpool_t *pool, const struct thrdpool_task *task);
int thrdpool_increase(thrdpool_t *pool);
int thrdpool_in_pool(thrdpool_t *pool);
void thrdpool_free(thrdpool_t *pool,
                   void (*pending)(const struct thrdpool_task *));

#endif
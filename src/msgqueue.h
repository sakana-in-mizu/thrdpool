#ifndef __MSGQUEUE_H__
#define __MSGQUEUE_H__

#include <stddef.h>

typedef struct __msgqueue msgqueue_t;

msgqueue_t *msgqueue_new(size_t maxlen, int linkoff);
void msgqueue_put(msgqueue_t *queue, void *msg);
void *msgqueue_get(msgqueue_t *queue);
void msgqueue_set_nonblock(msgqueue_t *queue);
void msgqueue_set_block(msgqueue_t *queue);
void msgqueue_free(msgqueue_t *queue);

#endif
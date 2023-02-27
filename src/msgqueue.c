#include "msgqueue.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

struct __msgqueue {
    size_t max_cnt;
    size_t msg_cnt;
    int linkoff;
    int nonblock;

    void *head1;
    void *head2;
    void **get_head;
    void **put_head;
    void **put_tail;

    pthread_mutex_t get_mutex;
    pthread_mutex_t put_mutex;
    pthread_cond_t get_cond;
    pthread_cond_t put_cond;
};

void msgqueue_set_block(msgqueue_t *queue) { queue->nonblock = 0; }

void msgqueue_set_nonblock(msgqueue_t *queue) {
    queue->nonblock = 1;
    pthread_mutex_lock(&queue->put_mutex);
    pthread_cond_signal(&queue->get_cond);
    pthread_cond_broadcast(&queue->put_cond);
    pthread_mutex_unlock(&queue->put_mutex);
}

msgqueue_t *msgqueue_new(size_t maxlen, int linkoff) {
    msgqueue_t *queue = malloc(sizeof(msgqueue_t));
    int ret;
    if (queue == NULL) {
        return NULL;
    }

    ret = pthread_mutex_init(&queue->get_mutex, NULL);
    if (ret == 0) {
        ret = pthread_mutex_init(&queue->put_mutex, NULL);
        if (ret == 0) {
            ret = pthread_cond_init(&queue->get_cond, NULL);
            if (ret == 0) {
                ret = pthread_cond_init(&queue->put_cond, NULL);
                if (ret == 0) {
                    queue->max_cnt = maxlen;
                    queue->linkoff = linkoff;
                    queue->msg_cnt = 0;
                    queue->nonblock = 0;

                    queue->head1 = NULL;
                    queue->head2 = NULL;
                    queue->get_head = &queue->head1;
                    queue->put_head = &queue->head2;
                    queue->put_tail = &queue->head2;

                    return queue;
                }

                pthread_cond_destroy(&queue->get_cond);
            }

            pthread_mutex_destroy(&queue->put_mutex);
        }

        pthread_mutex_destroy(&queue->get_mutex);
    }

    errno = ret;
    free(queue);
    return NULL;
}

void msgqueue_free(msgqueue_t *queue) {
    pthread_mutex_destroy(&queue->get_mutex);
    pthread_mutex_destroy(&queue->put_mutex);
    pthread_cond_destroy(&queue->get_cond);
    pthread_cond_destroy(&queue->put_cond);
    free(queue);
}

void msgqueue_put(msgqueue_t *queue, void *msg) {
    void **link = (void **)((char *)msg + queue->linkoff);

    *link = NULL;
    pthread_mutex_lock(&queue->put_mutex);
    while (queue->msg_cnt >= queue->max_cnt && !queue->nonblock) {
        pthread_cond_wait(&queue->put_cond, &queue->put_mutex);
    }

    *queue->put_tail = link;
    queue->put_tail = link;
    ++queue->msg_cnt;

    pthread_cond_signal(&queue->get_cond);
    pthread_mutex_unlock(&queue->put_mutex);
}

size_t __msgqueue_swap(msgqueue_t *queue) {
    void **get_head = queue->get_head;
    size_t cnt;

    queue->get_head = queue->put_head;

    pthread_mutex_lock(&queue->put_mutex);
    
    while (queue->msg_cnt == 0 && !queue->nonblock) {
        pthread_cond_wait(&queue->get_cond, &queue->put_mutex);
    }

    cnt = queue->msg_cnt;
    if (cnt >= queue->max_cnt) {
        pthread_cond_broadcast(&queue->put_cond);
    }

    queue->put_head = get_head;
    queue->put_tail = get_head;
    queue->msg_cnt = 0;

    pthread_mutex_unlock(&queue->put_mutex);

    return cnt;
}

void *msgqueue_get(msgqueue_t *queue) {
    void *msg;

    pthread_mutex_lock(&queue->get_mutex);

    if (*queue->get_head != NULL || __msgqueue_swap(queue) > 0) {
        msg = (char *)*queue->get_head - queue->linkoff;
        *queue->get_head = *(void **)*queue->get_head;
    } else {
        msg = NULL;
        errno = EAGAIN;
    }

    pthread_mutex_unlock(&queue->get_mutex);
    return msg;
}
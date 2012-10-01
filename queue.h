#ifndef QUEUE_H
#define QUEUE_H

/* FIFO queue.  Functions like queue_get handle disabling/restoring
   interrupts automatically; prefixed versions like _queue_get do
   not.  */

/* This is kind of a mess, because it's been turned to inline mush in
   order to improve speed. */

#include <stdint.h>
#include <util/atomic.h>

#define __QUEUE_NEXT(q, x) (((q->x+1) == q->len) ? 0 : (q->x+1))

struct queue {
	volatile int len;
	volatile uint8_t *data;
	volatile int start;
	volatile int end;
};

/* Initialize the queue "q"
   Pass the data buffer and maximum length to use */
static inline void queue_init(struct queue *q, uint8_t *data, int len)
{
	q->len = len;
	q->data = data;
	q->start = 0;
	q->end = 0;
}

/* Macro form of initializer */
#define QUEUE_INITIALIZER(thedata, thelen) { .data = thedata, .len = thelen }

/* Get a byte from queue.  Returns -1 if queue empty */
static inline int _queue_get(struct queue *q)
{
	int ret = -1;
	if (q->start != q->end) {
		ret = q->data[q->start];
		q->start = __QUEUE_NEXT(q, start);
	}
	return ret;
}
static inline int queue_get(struct queue *q)
{
	int ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = _queue_get(q);
	}
	return ret;
}

/* Put a byte in queue.  Returns -1 if queue full, 0 otherwise */
static inline int _queue_put(struct queue *q, uint8_t v)
{
	int next = __QUEUE_NEXT(q, end);
	if (next != q->start) {
		q->data[q->end] = v;
		q->end = next;
		return 0;
	}
	return -1;
}
static inline int queue_put(struct queue *q, uint8_t v)
{
	int ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = _queue_put(q, v);
	}
	return ret;
}

/* Returns 1 if the operation would succeed, 0 otherwise */
static inline int _queue_can_get(struct queue *q)
{
	return (q->start != q->end) ? 1 : 0;
}
static inline int _queue_can_put(struct queue *q)
{
	return (__QUEUE_NEXT(q, end) != q->start) ? 1 : 0;

}
static inline int queue_can_get(struct queue *q)
{
	int ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = _queue_can_get(q);
	}
	return ret;
}
static inline int queue_can_put(struct queue *q)
{
	int ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = _queue_can_put(q);
	}
	return ret;
}

/* Returns number of bytes in the queue */
static inline int _queue_get_len(struct queue *q)
{
	int len = q->end - q->start;
	if (len < 0)
		len += q->len;
	return len;
}

static inline int queue_get_len(struct queue *q)
{
	int ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = _queue_get_len(q);
	}
	return ret;
}

/* Returns number of bytes FREE in the queue */
static inline int _queue_get_free(struct queue *q)
{
	/* If q->len is 64, we can only fit 63 bytes */
	return q->len - _queue_get_len(q) - 1;
}
static inline int queue_get_free(struct queue *q)
{
	int ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = _queue_get_free(q);
	}
	return ret;
}

#endif

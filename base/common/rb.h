#ifndef COMMON_RB_H
#define COMMON_RB_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cmsis_compiler.h"

/*
 * Single-producer / single-consumer byte ring.
 *
 * size must be a power of two.  One slot is kept unused so that full and empty
 * are distinguishable.  The producer owns head, the consumer owns tail.
 */
typedef struct
{
    uint8_t *buf;
    uint32_t size;
    uint32_t mask;
    volatile uint32_t head;
    volatile uint32_t tail;
} ringbuf_t;

static inline bool rb_is_empty(const ringbuf_t *rb)
{
    return rb->head == rb->tail;
}

static inline bool rb_is_full(const ringbuf_t *rb)
{
    return ((rb->head + 1U) & rb->mask) == rb->tail;
}

static inline uint32_t rb_count(const ringbuf_t *rb)
{
    return (rb->head - rb->tail) & rb->mask;
}

static inline uint32_t rb_space(const ringbuf_t *rb)
{
    return rb->mask - rb_count(rb);
}

static inline int rb_init(ringbuf_t *rb, uint8_t *storage, uint32_t size)
{
    if ((rb == 0) || (storage == 0) ||
        (size < 2U) || ((size & (size - 1U)) != 0U))
    {
        return -1;
    }

    rb->buf = storage;
    rb->size = size;
    rb->mask = size - 1U;
    rb->head = 0U;
    rb->tail = 0U;
    return 0;
}

static inline uint32_t rb_put(ringbuf_t *rb, const uint8_t *data, uint32_t len)
{
    uint32_t space;
    uint32_t head;
    uint32_t first;

    if ((rb->size == 0U) || (data == 0) || (len == 0U))
    {
        return 0U;
    }

    space = rb_space(rb);
    if (len > space)
    {
        len = space;
    }
    if (len == 0U)
    {
        return 0U;
    }

    head = rb->head & rb->mask;
    first = rb->size - head;
    if (first > len)
    {
        first = len;
    }
    memcpy(&rb->buf[head], data, first);
    if (len > first)
    {
        memcpy(rb->buf, data + first, len - first);
    }

    __DMB();
    rb->head = (head + len) & rb->mask;
    return len;
}

static inline uint32_t rb_get(ringbuf_t *rb, uint8_t *data, uint32_t len)
{
    uint32_t used;
    uint32_t tail;
    uint32_t first;

    if ((rb->size == 0U) || (data == 0) || (len == 0U))
    {
        return 0U;
    }

    used = rb_count(rb);
    if (len > used)
    {
        len = used;
    }
    if (len == 0U)
    {
        return 0U;
    }

    tail = rb->tail & rb->mask;
    first = rb->size - tail;
    if (first > len)
    {
        first = len;
    }
    memcpy(data, &rb->buf[tail], first);
    if (len > first)
    {
        memcpy(data + first, rb->buf, len - first);
    }

    __DMB();
    rb->tail = (tail + len) & rb->mask;
    return len;
}

static inline bool rb_put_byte(ringbuf_t *rb, uint8_t value)
{
    return rb_put(rb, &value, 1U) == 1U;
}

static inline bool rb_get_byte(ringbuf_t *rb, uint8_t *value)
{
    return rb_get(rb, value, 1U) == 1U;
}

#endif /* COMMON_RB_H */

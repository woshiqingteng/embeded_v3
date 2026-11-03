#ifndef COMMON_RINGBUF_H
#define COMMON_RINGBUF_H

#include <stddef.h>
#include <stdint.h>

/*
 * Single-producer / single-consumer byte ring.  The writer updates head after
 * writing; the reader updates tail after reading.  No lock is required when
 * there is exactly one writer and one reader.
 */
typedef struct
{
    uint8_t *buf;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
} ringbuf_t;

static inline void ringbuf_compiler_barrier(void)
{
    __asm volatile ("" ::: "memory");
}

static inline void ringbuf_init(ringbuf_t *rb, uint8_t *storage, uint32_t size)
{
    rb->buf  = storage;
    rb->size = size;
    rb->head = 0U;
    rb->tail = 0U;
}

static inline void ringbuf_reset(ringbuf_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
    ringbuf_compiler_barrier();
}

static inline uint32_t ringbuf_used(const ringbuf_t *rb)
{
    uint32_t head = rb->head % rb->size;
    uint32_t tail = rb->tail % rb->size;

    return (head >= tail) ? (head - tail) : (rb->size - tail + head);
}

static inline uint32_t ringbuf_free(const ringbuf_t *rb)
{
    return rb->size - ringbuf_used(rb);
}

static inline uint32_t ringbuf_put(ringbuf_t *rb, const uint8_t *data, uint32_t len)
{
    uint32_t space = ringbuf_free(rb);
    uint32_t head;

    if (len > space)
    {
        len = space;
    }

    head = rb->head;
    for (uint32_t i = 0U; i < len; i++)
    {
        rb->buf[(head + i) % rb->size] = data[i];
    }

    ringbuf_compiler_barrier();
    rb->head = (head + len) % rb->size;
    return len;
}

static inline uint32_t ringbuf_get(ringbuf_t *rb, uint8_t *data, uint32_t len)
{
    uint32_t used = ringbuf_used(rb);
    uint32_t tail;

    if (len > used)
    {
        len = used;
    }

    tail = rb->tail;
    for (uint32_t i = 0U; i < len; i++)
    {
        data[i] = rb->buf[(tail + i) % rb->size];
    }

    ringbuf_compiler_barrier();
    rb->tail = (tail + len) % rb->size;
    return len;
}

static inline int ringbuf_put_byte(ringbuf_t *rb, uint8_t value)
{
    return ringbuf_put(rb, &value, 1U) == 1U;
}

static inline int ringbuf_get_byte(ringbuf_t *rb, uint8_t *value)
{
    return ringbuf_get(rb, value, 1U) == 1U;
}

#endif /* COMMON_RINGBUF_H */

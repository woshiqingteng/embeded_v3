#ifndef COMMON_BITOPS_H
#define COMMON_BITOPS_H

#include <stddef.h>
#include <stdint.h>

#define BIT(n)                  (1UL << (n))
#define BIT_MASK(n)             (BIT(n) - 1UL)
#define SET_BITS(reg, mask)     ((reg) |= (mask))
#define CLR_BITS(reg, mask)     ((reg) &= ~(mask))
#define READ_BITS(reg, mask)    ((reg) & (mask))


#define BIT_SET(reg, n)      ((reg) |=  (1UL << (n)))
#define BIT_CLR(reg, n)      ((reg) &= ~(1UL << (n)))
#define TOGGLE_BIT(reg, n)   ((reg) ^=  (1UL << (n)))
#define CHECK_BIT(reg, n)    (((reg) >> (n)) & 1UL)
#define REG32(addr)          (*(volatile uint32_t *)(uintptr_t)(addr))

#define BF_MASK(width)       ((uint32_t)(~0ULL >> (32U - (width))))
#define BF_GET(reg, pos, width) \
    (((reg) >> (pos)) & BF_MASK(width))
#define BF_SET(reg, pos, width, val) \
    ((reg) = ((reg) & ~(BF_MASK(width) << (pos))) | \
             (((val) & BF_MASK(width)) << (pos)))

#define ARRAY_SIZE(a)           (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))
#define CLAMP(v, lo, hi)        (MIN(MAX((v), (lo)), (hi)))

#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

static inline int bitops_is_power_of_two(uint32_t v)
{
    return (v != 0U) && ((v & (v - 1U)) == 0U);
}

static inline uint32_t bitops_next_power_of_two(uint32_t v)
{
    if (v == 0U)
    {
        return 1U;
    }

    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1U;
}

#endif /* COMMON_BITOPS_H */

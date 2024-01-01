#ifndef H_UTILS_H

#include <stdint.h>
#include <stddef.h>

typedef int8_t     i8;
typedef int16_t    i16;
typedef int32_t    i32;
typedef int64_t    i64;

typedef uint8_t    u8;
typedef uint16_t   u16;
typedef uint32_t   u32;
typedef uint64_t   u64;

typedef int32_t    b32; // NOTE: For bool.

typedef size_t    memory_size;

typedef float      f32;
typedef double     f64;

#undef TRUE
#undef FALSE
#define TRUE       (1)
#define FALSE      (0)

void platform_throw_error(const u8* text);

#define ASSERT(x) do { if (!(x)) { platform_throw_error((u8*)#x); *(volatile int*)0; } } while (0)
#define ARRAY_COUNT(x)      (sizeof(x) / sizeof(*(x)))

#define SWAP(a, b, t)                           \
    {                                           \
        t temp = a;                             \
        a = b;                                  \
        b = temp;                               \
    }

#define DOUBLY_LINKED_LIST_INIT(sentinel)       \
    (sentinel)->prev = (sentinel);              \
    (sentinel)->next = (sentinel);

#define DOUBLY_LINKED_LIST_INSERT(sentinel, element)    \
    (element)->next = (sentinel)->next;                 \
    (element)->prev = (sentinel);                       \
    (element)->next->prev = (element);                 \
    (element)->prev->next = (element);

static inline f32 lerp(f32 min, f32 value, f32 max)
{
    f32 result = min + value * (max - min);

    return result;
}

static inline i32 modulo(i32 a, i32 b)
{
    return (a % b + b) % b;
}

#define H_UTILS_H
#endif

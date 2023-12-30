#ifndef DD_DEFINES_H
#define DD_DEFINES_H

#include "config.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef float f32;
typedef double f64;

typedef _Bool b8;
typedef u32 b32;

#if defined(__clang__) || defined(__gcc__) || defined(__GNUC__)
#define TYPEOF(var) __typeof__(var)
#elif defined(__TINYC__)
#define TYPEOF(var) typeof(var)
#else
#define TYPEOF(var) typeof(var)
#endif

#define true 1
#define false 0

#define OPTIMIZE_ENABLE __pragma(optimize ("", on))
#define OPTIMIZE_DISABLE __pragma(optimize ("", off))

#ifdef LIBRARY_EXPORTS
#if defined(_MSC_VER)
#define LIB_API __declspec(dllexport)
#else
#define LIB_API __attribute__((visibility("default")))
#endif
#else
#if defined(_MSC_VER)
#define LIB_API __declspec(dllimport)
#else
#define LIB_API
#endif
#endif

#define BIT(x_) (1llu << (x_))
#define CHANGE_BIT(_num, _index, _value) (_num ^= (-(_value) ^ _num) & (_index))
#define CHECK_BIT(_num, _index) ((_num >> _index) & 1U)
#define TOGGLE_BIT(_num, _index) (_num ^= 1UL << _index)
#define SET_BIT(_num, _index) (_num |= 1UL << _index)
#define CLEAR_BIT(_num, _index) (_num &= ~(1UL << _index))

#define GiB (1024 * 1024 * 1024)
#define MiB (1024 * 1024)
#define KiB (1024)

#define sec2ms (1000)
#define sec2us (1000000)
#define sec2ns (1000000000)

#define ptr2bool *(bool*)
#define ptr2(type) *(type*)

#define cast_ptr(type) (type*)

#define FLT_MAX 340282346638528859811704183484516925440.0f
#define INT_MAX 2147483647
#define TAU 2 * PI

#define CLAMP(value_, min_, max_) (value_ <= min_) ? min_ : (value_ >= max_) ? max_ : value_;
#define MIN(v1_, v2_) (v1_ < v2_ ? v1_ : v2_)
#define MAX(v1_, v2_) (v1_ > v2_ ? v1_ : v2_)
#define SWAP(v1_, v2_) do {\
TYPEOF(v1_) vt_ = v1_;\
v1_ = v2_;\
v2_ = vt_; } while (0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define print_bits(x)\
do\
{\
u64 a__ = (x);\
u64 bits__ = sizeof(x) * 8;\
printf(#x ": ");\
while (bits__--) putchar(a__ &(1ULL << bits__) ? '1' : '0');\
putchar('\n');\
} while (0)

#if defined(__clang__) || defined(__gcc__)
#define LIB_INLINE __attribute__((always_inline)) inline
#define LIB_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define LIB_INLINE __forceinline
#define LIB_NOINLINE __declspec(noinline)
#else
#define LIB_INLINE static inline
#define LIB_NOINLINE
#endif

#endif

// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include <cstdint>

typedef int8_t    s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef s8        b8;
typedef s16      b16;
typedef s32      b32;
typedef s64      b64;

#define CORE_ASSERT(exp) if (!(exp)) { *(volatile int*)0 = 0; }

#ifdef _MSC_VER
#  define FORCE_INLINE __forceinline
#else
#  error Undefined compiler.
#endif

#ifdef PLATFORM_WINDOWS
#  define ENGINE_API __declspec(dllexport)
#else
#  error Undefined platform.
#endif

FORCE_INLINE void MemoryCopy(void* dst, void *src, auto sz)
{
    memcpy(dst, src, sz);
}

#define ARRAY_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))

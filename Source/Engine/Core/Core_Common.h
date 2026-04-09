// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include <cstdint>
#include <type_traits>

typedef int8_t           s8;
typedef int16_t         s16;
typedef int32_t         s32;
typedef int64_t         s64;
typedef uint8_t          u8;
typedef uint16_t        u16;
typedef uint32_t        u32;
typedef uint64_t        u64;
typedef s8               b8;
typedef s16             b16;
typedef s32             b32;
typedef unsigned int    uint;

typedef uint8_t          uint8;
typedef uint16_t         uint16;
typedef uint32_t         uint32;
typedef uint64_t         uint64;

#define INTERNAL static
#define INVALID_DEFAULT_CASE default: CORE_ASSERT(!"invalid default case") 

#define CORE_ASSERT(exp, ...) if (!(exp)) { *(volatile int*)0 = 0; }
#define ARRAY_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))

#ifdef _MSC_VER
#  define FORCE_INLINE __forceinline
#else
#  error Undefined compiler.
#endif

template <typename T>
FORCE_INLINE T AlignUp(T x, T align) 
{
    return (x + align - 1) & ~(align - 1);
}

template <typename T>
FORCE_INLINE T AlignDown(T x, T align) 
{
    return x & ~(align - 1);
}

FORCE_INLINE void MemorySet(void* dst, int value, size_t size)
{
    memset(dst, value, size);
}


#ifdef PLATFORM_WINDOWS
// Windows specific
//
#  define ENGINE_API __declspec(dllexport)

#include <unknwn.h>

template <typename T>
FORCE_INLINE void SafeReleaseCOM(T** ppCOM)
{
    if (ppCOM)
    {
        IUnknown* pCOM = *ppCOM;
        if (pCOM)
        {
            pCOM->Release();
            *ppCOM = nullptr;
        }
    }
}

#else
#  error Undefined platform.
#endif

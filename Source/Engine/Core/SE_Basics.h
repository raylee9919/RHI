// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include <cstdint>
#include <stack>
#include <type_traits>
#include <bit>

typedef int8_t          int8;
typedef int16_t        int16;
typedef int32_t        int32;
typedef int64_t        int64;
typedef uint8_t        uint8;
typedef uint16_t      uint16;
typedef uint32_t      uint32;
typedef uint64_t      uint64;
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
typedef float  f32;
typedef double f64;

#define INTERNAL static
#define INVALID_DEFAULT_CASE default: CORE_ASSERT(!"invalid default case") 
#define CONCAT(A, B) A##B
#define CONCAT2(A, B) CONCAT(A, B)

#define Min(a, b) (((a) > (b)) ? (b) : (a))
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Clamp(x, lo, hi) Max(Min((x), (hi)), (lo))

template <typename F>
struct Scope_Exit {
    Scope_Exit(F f) : f(f) {}
    ~Scope_Exit() { f(); }
    F f;
};
template <typename F>
Scope_Exit<F> scope_exit_make(F f) {
    return Scope_Exit<F>(f);
};
#define defer(code) \
    auto CONCAT2(scope_exit_, __LINE__) = scope_exit_make([=](){code;})


#if PLATFORM_WINDOWS
#  define CORE_ASSERT(exp, ...) if (!(exp)) { __debugbreak(); }
#  define assert(exp, ... )     if (!(exp)) { __debugbreak(); }
#else
#  define CORE_ASSERT(exp, ...) if (!(exp)) { *(volatile int*)0 = 0; }
#  define assert(exp, ... )     if (!(exp)) { *(volatile int*)0 = 0; }
#endif


#define ARRAY_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))

#ifdef _MSC_VER
#  define FORCE_INLINE __forceinline
#else
#  error Undefined compiler.
#endif

#ifdef PLATFORM_WINDOWS
// Windows specific
//
#  define ENGINE_API __declspec(dllexport)

#  define CORE_ASSERT_SUCCEEDED(hr) CORE_ASSERT(SUCCEEDED(hr))

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




namespace Engine
{
    template <typename T>
    FORCE_INLINE T align_up(T x, T align) 
    {
        return (x + align - 1) & ~(align - 1);
    }

    template <typename T>
    FORCE_INLINE T align_down(T x, T align) 
    {
        return x & ~(align - 1);
    }

    FORCE_INLINE int tzcnt(unsigned int x) {
        return std::countr_zero(x);
    }

    // Returns first encountered set bit's index (zero-indexed).
    // If not found, return 32.
    ENGINE_API uint32_t BitScanFromLSB(uint32_t x);


    static const f32 F32_MAX = 3.402823e+38f;
    static const f32 F32_MIN = -3.402823e+38f;



    
    template<typename T>
    using Stack = std::stack<T>;
}

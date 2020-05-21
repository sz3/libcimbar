/** \file
    \brief GF(256) Main C API Header
    \copyright Copyright (c) 2017 Christopher A. Taylor.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of GF256 nor the names of its contributors may be
      used to endorse or promote products derived from this software without
      specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CAT_GF256_H
#define CAT_GF256_H

/** \page GF256 GF(256) Math Module

    This module provides efficient implementations of bulk
    GF(2^^8) math operations over memory buffers.

    Addition is done over the base field in GF(2) meaning
    that addition is XOR between memory buffers.

    Multiplication is performed using table lookups via
    SIMD instructions.  This is somewhat slower than XOR,
    but fast enough to not become a major bottleneck when
    used sparingly.
*/

#include <stdint.h> // uint32_t etc
#include <cstring> // memcpy, memset

/// Library header version
#define GF256_VERSION 2

//------------------------------------------------------------------------------
// Platform/Architecture

#if defined(ANDROID) || defined(IOS) || defined(LINUX_ARM)
    #define GF256_TARGET_MOBILE
#endif // ANDROID

#if defined(__AVX2__) || (defined (_MSC_VER) && _MSC_VER >= 1900)
    #define GF256_TRY_AVX2 /* 256-bit */
    #include <immintrin.h>
    #define GF256_ALIGN_BYTES 32
#else // __AVX2__
    #define GF256_ALIGN_BYTES 16
#endif // __AVX2__

#if !defined(GF256_TARGET_MOBILE)
    // Note: MSVC currently only supports SSSE3 but not AVX2
    #include <tmmintrin.h> // SSSE3: _mm_shuffle_epi8
    #include <emmintrin.h> // SSE2
#endif // GF256_TARGET_MOBILE

#if defined(HAVE_ARM_NEON_H)
    #include <arm_neon.h>
#endif // HAVE_ARM_NEON_H

#if defined(GF256_TARGET_MOBILE)

    #define GF256_ALIGNED_ACCESSES /* Inputs must be aligned to GF256_ALIGN_BYTES */

# if defined(HAVE_ARM_NEON_H)
    // Compiler-specific 128-bit SIMD register keyword
    #define GF256_M128 uint8x16_t
    #define GF256_TRY_NEON
#else
    #define GF256_M128 uint64_t
# endif

#else // GF256_TARGET_MOBILE

    // Compiler-specific 128-bit SIMD register keyword
    #define GF256_M128 __m128i

#endif // GF256_TARGET_MOBILE

#ifdef GF256_TRY_AVX2
    // Compiler-specific 256-bit SIMD register keyword
    #define GF256_M256 __m256i
#endif

// Compiler-specific C++11 restrict keyword
#define GF256_RESTRICT __restrict

// Compiler-specific force inline keyword
#ifdef _MSC_VER
    #define GF256_FORCE_INLINE inline __forceinline
#else
    #define GF256_FORCE_INLINE inline __attribute__((always_inline))
#endif

// Compiler-specific alignment keyword
// Note: Alignment only matters for ARM NEON where it should be 16
#ifdef _MSC_VER
    #define GF256_ALIGNED __declspec(align(GF256_ALIGN_BYTES))
#else // _MSC_VER
    #define GF256_ALIGNED __attribute__((aligned(GF256_ALIGN_BYTES)))
#endif // _MSC_VER

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


//------------------------------------------------------------------------------
// Portability

/// Swap two memory buffers in-place
extern void gf256_memswap(void * GF256_RESTRICT vx, void * GF256_RESTRICT vy, int bytes);


//------------------------------------------------------------------------------
// GF(256) Context

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4324) // warning C4324: 'gf256_ctx' : structure was padded due to __declspec(align())
#endif // _MSC_VER

/// The context object stores tables required to perform library calculations
struct gf256_ctx
{
    /// We require memory to be aligned since the SIMD instructions benefit from
    /// or require aligned accesses to the table data.
    struct
    {
        GF256_ALIGNED GF256_M128 TABLE_LO_Y[256];
        GF256_ALIGNED GF256_M128 TABLE_HI_Y[256];
    } MM128;
#ifdef GF256_TRY_AVX2
    struct
    {
        GF256_ALIGNED GF256_M256 TABLE_LO_Y[256];
        GF256_ALIGNED GF256_M256 TABLE_HI_Y[256];
    } MM256;
#endif // GF256_TRY_AVX2

    /// Mul/Div/Inv/Sqr tables
    uint8_t GF256_MUL_TABLE[256 * 256];
    uint8_t GF256_DIV_TABLE[256 * 256];
    uint8_t GF256_INV_TABLE[256];
    uint8_t GF256_SQR_TABLE[256];

    /// Log/Exp tables
    uint16_t GF256_LOG_TABLE[256];
    uint8_t GF256_EXP_TABLE[512 * 2 + 1];

    /// Polynomial used
    unsigned Polynomial;
};

#ifdef _MSC_VER
    #pragma warning(pop)
#endif // _MSC_VER

extern gf256_ctx GF256Ctx;


//------------------------------------------------------------------------------
// Initialization

/**
    Initialize a context, filling in the tables.
    
    Thread-safety / Usage Notes:
    
    It is perfectly safe and encouraged to use a gf256_ctx object from multiple
    threads.  The gf256_init() is relatively expensive and should only be done
    once, though it will take less than a millisecond.
    
    The gf256_ctx object must be aligned to 16 byte boundary.
    Simply tag the object with GF256_ALIGNED to achieve this.
    
    Example:
       static GF256_ALIGNED gf256_ctx TheGF256Context;
       gf256_init(&TheGF256Context, 0);
    
    Returns 0 on success and other values on failure.
*/
extern int gf256_init_(int version);
#define gf256_init() gf256_init_(GF256_VERSION)


//------------------------------------------------------------------------------
// Math Operations

/// return x + y
static GF256_FORCE_INLINE uint8_t gf256_add(uint8_t x, uint8_t y)
{
    return (uint8_t)(x ^ y);
}

/// return x * y
/// For repeated multiplication by a constant, it is faster to put the constant in y.
static GF256_FORCE_INLINE uint8_t gf256_mul(uint8_t x, uint8_t y)
{
    return GF256Ctx.GF256_MUL_TABLE[((unsigned)y << 8) + x];
}

/// return x / y
/// Memory-access optimized for constant divisors in y.
static GF256_FORCE_INLINE uint8_t gf256_div(uint8_t x, uint8_t y)
{
    return GF256Ctx.GF256_DIV_TABLE[((unsigned)y << 8) + x];
}

/// return 1 / x
static GF256_FORCE_INLINE uint8_t gf256_inv(uint8_t x)
{
    return GF256Ctx.GF256_INV_TABLE[x];
}

/// return x * x
static GF256_FORCE_INLINE uint8_t gf256_sqr(uint8_t x)
{
    return GF256Ctx.GF256_SQR_TABLE[x];
}


//------------------------------------------------------------------------------
// Bulk Memory Math Operations

/// Performs "x[] += y[]" bulk memory XOR operation
extern void gf256_add_mem(void * GF256_RESTRICT vx,
                          const void * GF256_RESTRICT vy, int bytes);

/// Performs "z[] += x[] + y[]" bulk memory operation
extern void gf256_add2_mem(void * GF256_RESTRICT vz, const void * GF256_RESTRICT vx,
                           const void * GF256_RESTRICT vy, int bytes);

/// Performs "z[] = x[] + y[]" bulk memory operation
extern void gf256_addset_mem(void * GF256_RESTRICT vz, const void * GF256_RESTRICT vx,
                             const void * GF256_RESTRICT vy, int bytes);

/// Performs "z[] = x[] * y" bulk memory operation
extern void gf256_mul_mem(void * GF256_RESTRICT vz,
                          const void * GF256_RESTRICT vx, uint8_t y, int bytes);

/// Performs "z[] += x[] * y" bulk memory operation
extern void gf256_muladd_mem(void * GF256_RESTRICT vz, uint8_t y,
                             const void * GF256_RESTRICT vx, int bytes);

/// Performs "x[] /= y" bulk memory operation
static GF256_FORCE_INLINE void gf256_div_mem(void * GF256_RESTRICT vz,
                                             const void * GF256_RESTRICT vx, uint8_t y, int bytes)
{
    // Multiply by inverse
    gf256_mul_mem(vz, vx, y == 1 ? (uint8_t)1 : GF256Ctx.GF256_INV_TABLE[y], bytes);
}


//------------------------------------------------------------------------------
// Misc Operations

/// Swap two memory buffers in-place
extern void gf256_memswap(void * GF256_RESTRICT vx, void * GF256_RESTRICT vy, int bytes);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAT_GF256_H

/** \file
    \brief GF(256) Main C API Source
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

#include "gf256.h"

#ifdef LINUX_ARM
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <linux/auxvec.h>
#endif

//------------------------------------------------------------------------------
// Detect host byte order.
// This check works with GCC and LLVM; assume little-endian byte order when
// using any other compiler.
// The result is verified during initialization.
//
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) \
    && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define GF256_IS_BIG_ENDIAN
#endif

//------------------------------------------------------------------------------
// Workaround for ARMv7 that doesn't provide vqtbl1_*
// This comes from linux-raid (https://www.spinics.net/lists/raid/msg58403.html)
//
#ifdef GF256_TRY_NEON
#if __ARM_ARCH <= 7 && !defined(__aarch64__)
static GF256_FORCE_INLINE uint8x16_t vqtbl1q_u8(uint8x16_t a, uint8x16_t b)
{
    union {
        uint8x16_t    val;
        uint8x8x2_t    pair;
    } __a = { a };

    return vcombine_u8(vtbl2_u8(__a.pair, vget_low_u8(b)),
                       vtbl2_u8(__a.pair, vget_high_u8(b)));
}
#endif
#endif

//------------------------------------------------------------------------------
// Self-Test
//
// This is executed during initialization to make sure the library is working

static const unsigned kTestBufferBytes = 32 + 16 + 8 + 4 + 2 + 1;
static const unsigned kTestBufferAllocated = 64;
struct SelfTestBuffersT
{
    GF256_ALIGNED uint8_t A[kTestBufferAllocated];
    GF256_ALIGNED uint8_t B[kTestBufferAllocated];
    GF256_ALIGNED uint8_t C[kTestBufferAllocated];
};
static GF256_ALIGNED SelfTestBuffersT m_SelfTestBuffers;

static bool gf256_self_test()
{
    if ((uintptr_t)m_SelfTestBuffers.A % GF256_ALIGN_BYTES != 0)
        return false;
    if ((uintptr_t)m_SelfTestBuffers.A % GF256_ALIGN_BYTES != 0)
        return false;
    if ((uintptr_t)m_SelfTestBuffers.B % GF256_ALIGN_BYTES != 0)
        return false;
    if ((uintptr_t)m_SelfTestBuffers.C % GF256_ALIGN_BYTES != 0)
        return false;

    // Check multiplication/division
    for (unsigned i = 0; i < 256; ++i)
    {
        for (unsigned j = 0; j < 256; ++j)
        {
            uint8_t prod = gf256_mul((uint8_t)i, (uint8_t)j);
            if (i != 0 && j != 0)
            {
                uint8_t div1 = gf256_div(prod, (uint8_t)i);
                if (div1 != j)
                    return false;
                uint8_t div2 = gf256_div(prod, (uint8_t)j);
                if (div2 != i)
                    return false;
            }
            else if (prod != 0)
                return false;
            if (j == 1 && prod != i)
                return false;
        }
    }

    // Check for overruns
    m_SelfTestBuffers.A[kTestBufferBytes] = 0x5a;
    m_SelfTestBuffers.B[kTestBufferBytes] = 0x5a;
    m_SelfTestBuffers.C[kTestBufferBytes] = 0x5a;

    // Test gf256_add_mem()
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
    {
        m_SelfTestBuffers.A[i] = 0x1f;
        m_SelfTestBuffers.B[i] = 0xf7;
    }
    gf256_add_mem(m_SelfTestBuffers.A, m_SelfTestBuffers.B, kTestBufferBytes);
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
        if (m_SelfTestBuffers.A[i] != (0x1f ^ 0xf7))
            return false;

    // Test gf256_add2_mem()
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
    {
        m_SelfTestBuffers.A[i] = 0x1f;
        m_SelfTestBuffers.B[i] = 0xf7;
        m_SelfTestBuffers.C[i] = 0x71;
    }
    gf256_add2_mem(m_SelfTestBuffers.A, m_SelfTestBuffers.B, m_SelfTestBuffers.C, kTestBufferBytes);
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
        if (m_SelfTestBuffers.A[i] != (0x1f ^ 0xf7 ^ 0x71))
            return false;

    // Test gf256_addset_mem()
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
    {
        m_SelfTestBuffers.A[i] = 0x55;
        m_SelfTestBuffers.B[i] = 0xaa;
        m_SelfTestBuffers.C[i] = 0x6c;
    }
    gf256_addset_mem(m_SelfTestBuffers.A, m_SelfTestBuffers.B, m_SelfTestBuffers.C, kTestBufferBytes);
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
        if (m_SelfTestBuffers.A[i] != (0xaa ^ 0x6c))
            return false;

    // Test gf256_muladd_mem()
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
    {
        m_SelfTestBuffers.A[i] = 0xff;
        m_SelfTestBuffers.B[i] = 0xaa;
    }
    const uint8_t expectedMulAdd = gf256_mul(0xaa, 0x6c);
    gf256_muladd_mem(m_SelfTestBuffers.A, 0x6c, m_SelfTestBuffers.B, kTestBufferBytes);
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
        if (m_SelfTestBuffers.A[i] != (expectedMulAdd ^ 0xff))
            return false;

    // Test gf256_mul_mem()
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
    {
        m_SelfTestBuffers.A[i] = 0xff;
        m_SelfTestBuffers.B[i] = 0x55;
    }
    const uint8_t expectedMul = gf256_mul(0xa2, 0x55);
    gf256_mul_mem(m_SelfTestBuffers.A, m_SelfTestBuffers.B, 0xa2, kTestBufferBytes);
    for (unsigned i = 0; i < kTestBufferBytes; ++i)
        if (m_SelfTestBuffers.A[i] != expectedMul)
            return false;

    if (m_SelfTestBuffers.A[kTestBufferBytes] != 0x5a)
        return false;
    if (m_SelfTestBuffers.B[kTestBufferBytes] != 0x5a)
        return false;
    if (m_SelfTestBuffers.C[kTestBufferBytes] != 0x5a)
        return false;

    return true;
}


//------------------------------------------------------------------------------
// Runtime CPU Architecture Check
//
// Feature checks stolen shamelessly from
// https://github.com/jedisct1/libsodium/blob/master/src/libsodium/sodium/runtime.c

#if defined(HAVE_ANDROID_GETCPUFEATURES)
#include <cpu-features.h>
#endif

#if defined(GF256_TRY_NEON)
# if defined(IOS) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
// Requires iPhone 5S or newer
static const bool CpuHasNeon = true;
static const bool CpuHasNeon64 = true;
# else // ANDROID or LINUX_ARM
#  if defined(__aarch64__)
static bool CpuHasNeon = true;      // if AARCH64, then we have NEON for sure...
static bool CpuHasNeon64 = true;    // And we have ASIMD
#  else
static bool CpuHasNeon = false;     // if not, then we have to check at runtime.
static bool CpuHasNeon64 = false;   // And we don't have ASIMD
#  endif
# endif
#endif

#if !defined(GF256_TARGET_MOBILE)

#ifdef _MSC_VER
    #include <intrin.h> // __cpuid
    #pragma warning(disable: 4752) // found Intel(R) Advanced Vector Extensions; consider using /arch:AVX
#endif

#ifdef GF256_TRY_AVX2
static bool CpuHasAVX2 = false;
#endif
static bool CpuHasSSSE3 = false;

#define CPUID_EBX_AVX2    0x00000020
#define CPUID_ECX_SSSE3   0x00000200

static void _cpuid(unsigned int cpu_info[4U], const unsigned int cpu_info_type)
{
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86))
    __cpuid((int *) cpu_info, cpu_info_type);
#else //if defined(HAVE_CPUID)
    cpu_info[0] = cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
# ifdef __i386__
    __asm__ __volatile__ ("pushfl; pushfl; "
                          "popl %0; "
                          "movl %0, %1; xorl %2, %0; "
                          "pushl %0; "
                          "popfl; pushfl; popl %0; popfl" :
                          "=&r" (cpu_info[0]), "=&r" (cpu_info[1]) :
                          "i" (0x200000));
    if (((cpu_info[0] ^ cpu_info[1]) & 0x200000) == 0) {
        return; /* LCOV_EXCL_LINE */
    }
# endif
# ifdef __i386__
    __asm__ __volatile__ ("xchgl %%ebx, %k1; cpuid; xchgl %%ebx, %k1" :
                          "=a" (cpu_info[0]), "=&r" (cpu_info[1]),
                          "=c" (cpu_info[2]), "=d" (cpu_info[3]) :
                          "0" (cpu_info_type), "2" (0U));
# elif defined(__x86_64__)
    __asm__ __volatile__ ("xchgq %%rbx, %q1; cpuid; xchgq %%rbx, %q1" :
                          "=a" (cpu_info[0]), "=&r" (cpu_info[1]),
                          "=c" (cpu_info[2]), "=d" (cpu_info[3]) :
                          "0" (cpu_info_type), "2" (0U));
# else
    __asm__ __volatile__ ("cpuid" :
                          "=a" (cpu_info[0]), "=b" (cpu_info[1]),
                          "=c" (cpu_info[2]), "=d" (cpu_info[3]) :
                          "0" (cpu_info_type), "2" (0U));
# endif
#endif
}

#else
#if defined(LINUX_ARM)
static void checkLinuxARMNeonCapabilities( bool& cpuHasNeon )
{
    auto cpufile = open("/proc/self/auxv", O_RDONLY);
    Elf32_auxv_t auxv;
    if (cpufile >= 0)
    {
        const auto size_auxv_t = sizeof(Elf32_auxv_t);
        while (read(cpufile, &auxv, size_auxv_t) == size_auxv_t)
        {
            if (auxv.a_type == AT_HWCAP)
            {
                cpuHasNeon = (auxv.a_un.a_val & 4096) != 0;
                break;
            }
        }
        close(cpufile);
    }
    else
    {
        cpuHasNeon = false;
    }
}
#endif
#endif // defined(GF256_TARGET_MOBILE)

static void gf256_architecture_init()
{
#if defined(GF256_TRY_NEON)

    // Check for NEON support on Android platform
#if defined(HAVE_ANDROID_GETCPUFEATURES)
    AndroidCpuFamily family = android_getCpuFamily();
    if (family == ANDROID_CPU_FAMILY_ARM)
    {
        if (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON)
            CpuHasNeon = true;
    }
    else if (family == ANDROID_CPU_FAMILY_ARM64)
    {
        CpuHasNeon = true;
        if (android_getCpuFeatures() & ANDROID_CPU_ARM64_FEATURE_ASIMD)
            CpuHasNeon64 = true;
    }
#endif

#if defined(LINUX_ARM)
    // Check for NEON support on other ARM/Linux platforms
    checkLinuxARMNeonCapabilities(CpuHasNeon);
#endif

#endif //GF256_TRY_NEON

#if !defined(GF256_TARGET_MOBILE)
    unsigned int cpu_info[4];

    _cpuid(cpu_info, 1);
    CpuHasSSSE3 = ((cpu_info[2] & CPUID_ECX_SSSE3) != 0);

#if defined(GF256_TRY_AVX2)
    _cpuid(cpu_info, 7);
    CpuHasAVX2 = ((cpu_info[1] & CPUID_EBX_AVX2) != 0);
#endif // GF256_TRY_AVX2

    // When AVX2 and SSSE3 are unavailable, Siamese takes 4x longer to decode
    // and 2.6x longer to encode.  Encoding requires a lot more simple XOR ops
    // so it is still pretty fast.  Decoding is usually really quick because
    // average loss rates are low, but when needed it requires a lot more
    // GF multiplies requiring table lookups which is slower.

#endif // GF256_TARGET_MOBILE
}


//------------------------------------------------------------------------------
// Context Object

// Context object for GF(2^^8) math
GF256_ALIGNED gf256_ctx GF256Ctx;
static bool Initialized = false;


//------------------------------------------------------------------------------
// Generator Polynomial

// There are only 16 irreducible polynomials for GF(2^^8)
static const int GF256_GEN_POLY_COUNT = 16;
static const uint8_t GF256_GEN_POLY[GF256_GEN_POLY_COUNT] = {
    0x8e, 0x95, 0x96, 0xa6, 0xaf, 0xb1, 0xb2, 0xb4,
    0xb8, 0xc3, 0xc6, 0xd4, 0xe1, 0xe7, 0xf3, 0xfa
};

static const int kDefaultPolynomialIndex = 3;

// Select which polynomial to use
static void gf256_poly_init(int polynomialIndex)
{
    if (polynomialIndex < 0 || polynomialIndex >= GF256_GEN_POLY_COUNT)
        polynomialIndex = kDefaultPolynomialIndex;

    GF256Ctx.Polynomial = (GF256_GEN_POLY[polynomialIndex] << 1) | 1;
}


//------------------------------------------------------------------------------
// Exponential and Log Tables

// Construct EXP and LOG tables from polynomial
static void gf256_explog_init()
{
    unsigned poly = GF256Ctx.Polynomial;
    uint8_t* exptab = GF256Ctx.GF256_EXP_TABLE;
    uint16_t* logtab = GF256Ctx.GF256_LOG_TABLE;

    logtab[0] = 512;
    exptab[0] = 1;
    for (unsigned jj = 1; jj < 255; ++jj)
    {
        unsigned next = (unsigned)exptab[jj - 1] * 2;
        if (next >= 256)
            next ^= poly;

        exptab[jj] = static_cast<uint8_t>( next );
        logtab[exptab[jj]] = static_cast<uint16_t>( jj );
    }
    exptab[255] = exptab[0];
    logtab[exptab[255]] = 255;
    for (unsigned jj = 256; jj < 2 * 255; ++jj)
        exptab[jj] = exptab[jj % 255];
    exptab[2 * 255] = 1;
    for (unsigned jj = 2 * 255 + 1; jj < 4 * 255; ++jj)
        exptab[jj] = 0;
}


//------------------------------------------------------------------------------
// Multiply and Divide Tables

// Initialize MUL and DIV tables using LOG and EXP tables
static void gf256_muldiv_init()
{
    // Allocate table memory 65KB x 2
    uint8_t* m = GF256Ctx.GF256_MUL_TABLE;
    uint8_t* d = GF256Ctx.GF256_DIV_TABLE;

    // Unroll y = 0 subtable
    for (int x = 0; x < 256; ++x)
        m[x] = d[x] = 0;

    // For each other y value:
    for (int y = 1; y < 256; ++y)
    {
        // Calculate log(y) for mult and 255 - log(y) for div
        const uint8_t log_y = static_cast<uint8_t>(GF256Ctx.GF256_LOG_TABLE[y]);
        const uint8_t log_yn = 255 - log_y;

        // Next subtable
        m += 256, d += 256;

        // Unroll x = 0
        m[0] = 0, d[0] = 0;

        // Calculate x * y, x / y
        for (int x = 1; x < 256; ++x)
        {
            uint16_t log_x = GF256Ctx.GF256_LOG_TABLE[x];

            m[x] = GF256Ctx.GF256_EXP_TABLE[log_x + log_y];
            d[x] = GF256Ctx.GF256_EXP_TABLE[log_x + log_yn];
        }
    }
}


//------------------------------------------------------------------------------
// Inverse Table

// Initialize INV table using DIV table
static void gf256_inv_init()
{
    for (int x = 0; x < 256; ++x)
        GF256Ctx.GF256_INV_TABLE[x] = gf256_div(1, static_cast<uint8_t>(x));
}


//------------------------------------------------------------------------------
// Square Table

// Initialize SQR table using MUL table
static void gf256_sqr_init()
{
    for (int x = 0; x < 256; ++x)
        GF256Ctx.GF256_SQR_TABLE[x] = gf256_mul(static_cast<uint8_t>(x), static_cast<uint8_t>(x));
}


//------------------------------------------------------------------------------
// Multiply and Add Memory Tables

/*
    Fast algorithm to compute m[1..8] = a[1..8] * b in GF(256)
    using SSE3 SIMD instruction set:

    Consider z = x * y in GF(256).
    This operation can be performed bit-by-bit.  Usefully, the partial product
    of each bit is combined linearly with the rest.  This means that the 8-bit
    number x can be split into its high and low 4 bits, and partial products
    can be formed from each half.  Then the halves can be linearly combined:

        z = x[0..3] * y + x[4..7] * y

    The multiplication of each half can be done efficiently via table lookups,
    and the addition in GF(256) is XOR.  There must be two tables that map 16
    input elements for the low or high 4 bits of x to the two partial products.
    Each value for y has a different set of two tables:

        z = TABLE_LO_y(x[0..3]) xor TABLE_HI_y(x[4..7])

    This means that we need 16 * 2 * 256 = 8192 bytes for precomputed tables.

    Computing z[] = x[] * y can be performed 16 bytes at a time by using the
    128-bit register operations supported by modern processors.

    This is efficiently realized in SSE3 using the _mm_shuffle_epi8() function
    provided by Visual Studio 2010 or newer in <tmmintrin.h>.  This function
    uses the low bits to do a table lookup on each byte.  Unfortunately the
    high bit of each mask byte has the special feature that it clears the
    output byte when it is set, so we need to make sure it's cleared by masking
    off the high bit of each byte before using it:

        clr_mask = _mm_set1_epi8(0x0f) = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f

    For the low half of the partial product, clear the high bit of each byte
    and perform the table lookup:

        p_lo = _mm_and_si128(x, clr_mask)
        p_lo = _mm_shuffle_epi8(p_lo, TABLE_LO_y)

    For the high half of the partial product, shift the high 4 bits of each
    byte into the low 4 bits and clear the high bit of each byte, and then
    perform the table lookup:

        p_hi = _mm_srli_epi64(x, 4)
        p_hi = _mm_and_si128(p_hi, clr_mask)
        p_hi = _mm_shuffle_epi8(p_hi, TABLE_HI_y)

    Finally add the two partial products to form the product, recalling that
    addition is XOR in a Galois field:

        result = _mm_xor_si128(p_lo, p_hi)

    This crunches 16 bytes of x at a time, and the result can be stored in z.
*/

/*
    Intrinsic reference:

    SSE3, VS2010+, tmmintrin.h:

    GF256_M128 _mm_shuffle_epi8(GF256_M128 a, GF256_M128 mask);
        Emits the Supplemental Streaming SIMD Extensions 3 (SSSE3) instruction pshufb. This instruction shuffles 16-byte parameters from a 128-bit parameter.

        Pseudo-code for PSHUFB (with 128 bit operands):

            for i = 0 to 15 {
                 if (SRC[(i * 8)+7] = 1 ) then
                      DEST[(i*8)+7..(i*8)+0] <- 0;
                  else
                      index[3..0] <- SRC[(i*8)+3 .. (i*8)+0];
                      DEST[(i*8)+7..(i*8)+0] <- DEST[(index*8+7)..(index*8+0)];
                 endif
            }

    SSE2, VS2008+, emmintrin.h:

    GF256_M128 _mm_slli_epi64 (GF256_M128 a, int count);
        Shifts the 2 signed or unsigned 64-bit integers in a left by count bits while shifting in zeros.
    GF256_M128 _mm_srli_epi64 (GF256_M128 a, int count);
        Shifts the 2 signed or unsigned 64-bit integers in a right by count bits while shifting in zeros.
    GF256_M128 _mm_set1_epi8 (char b);
        Sets the 16 signed 8-bit integer values to b.
    GF256_M128 _mm_and_si128 (GF256_M128 a, GF256_M128 b);
        Computes the bitwise AND of the 128-bit value in a and the 128-bit value in b.
    GF256_M128 _mm_xor_si128 ( GF256_M128 a, GF256_M128 b);
        Computes the bitwise XOR of the 128-bit value in a and the 128-bit value in b.
*/

// Initialize the multiplication tables using gf256_mul()
static void gf256_mul_mem_init()
{
    // Reuse aligned self test buffers to load table data
    uint8_t* lo = m_SelfTestBuffers.A;
    uint8_t* hi = m_SelfTestBuffers.B;

    for (int y = 0; y < 256; ++y)
    {
        // TABLE_LO_Y maps 0..15 to 8-bit partial product based on y.
        for (unsigned char x = 0; x < 16; ++x)
        {
            lo[x] = gf256_mul(x, static_cast<uint8_t>( y ));
            hi[x] = gf256_mul(x << 4, static_cast<uint8_t>( y ));
        }

#if defined(GF256_TRY_NEON)
        if (CpuHasNeon)
        {
            GF256Ctx.MM128.TABLE_LO_Y[y] = vld1q_u8(lo);
            GF256Ctx.MM128.TABLE_HI_Y[y] = vld1q_u8(hi);
        }
#elif !defined(GF256_TARGET_MOBILE)
        const GF256_M128 table_lo = _mm_loadu_si128((GF256_M128*)lo);
        const GF256_M128 table_hi = _mm_loadu_si128((GF256_M128*)hi);
        _mm_storeu_si128(GF256Ctx.MM128.TABLE_LO_Y + y, table_lo);
        _mm_storeu_si128(GF256Ctx.MM128.TABLE_HI_Y + y, table_hi);
# ifdef GF256_TRY_AVX2
        if (CpuHasAVX2)
        {
            const GF256_M256 table_lo2 = _mm256_broadcastsi128_si256(table_lo);
            const GF256_M256 table_hi2 = _mm256_broadcastsi128_si256(table_hi);
            _mm256_storeu_si256(GF256Ctx.MM256.TABLE_LO_Y + y, table_lo2);
            _mm256_storeu_si256(GF256Ctx.MM256.TABLE_HI_Y + y, table_hi2);
        }
# endif // GF256_TRY_AVX2
#endif // GF256_TARGET_MOBILE
    }
}


//------------------------------------------------------------------------------
// Initialization

#ifdef GF256_IS_BIG_ENDIAN
static unsigned char kEndianTestData[4] = { 1, 2, 3, 4 };
#else
static unsigned char kEndianTestData[4] = { 4, 3, 2, 1 };
#endif

union UnionType
{
    uint32_t IntValue;
    char CharArray[4];
};

static bool IsExpectedEndian()
{
    UnionType type;
    for (unsigned i = 0; i < 4; ++i)
        type.CharArray[i] = kEndianTestData[i];
    return 0x01020304 == type.IntValue;
}

extern "C" int gf256_init_(int version)
{
    if (version != GF256_VERSION)
        return -1; // User's header does not match library version.

    // Avoid multiple initialization
    if (Initialized)
        return 0;
    Initialized = true;

    if (!IsExpectedEndian())
        return -2; // Unexpected byte order.

    gf256_architecture_init();
    gf256_poly_init(kDefaultPolynomialIndex);
    gf256_explog_init();
    gf256_muldiv_init();
    gf256_inv_init();
    gf256_sqr_init();
    gf256_mul_mem_init();

    if (!gf256_self_test())
        return -3; // Self-test failed (perhaps untested configuration)

    return 0;
}


//------------------------------------------------------------------------------
// Operations

extern "C" void gf256_add_mem(void * GF256_RESTRICT vx,
                              const void * GF256_RESTRICT vy, int bytes)
{
    GF256_M128 * GF256_RESTRICT x16 = reinterpret_cast<GF256_M128 *>(vx);
    const GF256_M128 * GF256_RESTRICT y16 = reinterpret_cast<const GF256_M128 *>(vy);

#if defined(GF256_TARGET_MOBILE)
# if defined(GF256_TRY_NEON)
    // Handle multiples of 64 bytes
    if (CpuHasNeon)
    {
        while (bytes >= 64)
        {
            GF256_M128 x0 = vld1q_u8((uint8_t*) x16);
            GF256_M128 x1 = vld1q_u8((uint8_t*)(x16 + 1) );
            GF256_M128 x2 = vld1q_u8((uint8_t*)(x16 + 2) );
            GF256_M128 x3 = vld1q_u8((uint8_t*)(x16 + 3) );
            GF256_M128 y0 = vld1q_u8((uint8_t*)y16);
            GF256_M128 y1 = vld1q_u8((uint8_t*)(y16 + 1));
            GF256_M128 y2 = vld1q_u8((uint8_t*)(y16 + 2));
            GF256_M128 y3 = vld1q_u8((uint8_t*)(y16 + 3));

            vst1q_u8((uint8_t*)x16,     veorq_u8(x0, y0));
            vst1q_u8((uint8_t*)(x16 + 1), veorq_u8(x1, y1));
            vst1q_u8((uint8_t*)(x16 + 2), veorq_u8(x2, y2));
            vst1q_u8((uint8_t*)(x16 + 3), veorq_u8(x3, y3));

            bytes -= 64, x16 += 4, y16 += 4;
        }

        // Handle multiples of 16 bytes
        while (bytes >= 16)
        {
            GF256_M128 x0 = vld1q_u8((uint8_t*)x16);
            GF256_M128 y0 = vld1q_u8((uint8_t*)y16);

            vst1q_u8((uint8_t*)x16, veorq_u8(x0, y0));

            bytes -= 16, ++x16, ++y16;
        }
    }
    else
# endif // GF256_TRY_NEON
    {
        uint64_t * GF256_RESTRICT x8 = reinterpret_cast<uint64_t *>(x16);
        const uint64_t * GF256_RESTRICT y8 = reinterpret_cast<const uint64_t *>(y16);

        const unsigned count = (unsigned)bytes / 8;
        for (unsigned ii = 0; ii < count; ++ii)
            x8[ii] ^= y8[ii];

        x16 = reinterpret_cast<GF256_M128 *>(x8 + count);
        y16 = reinterpret_cast<const GF256_M128 *>(y8 + count);

        bytes -= (count * 8);
    }
#else // GF256_TARGET_MOBILE
# if defined(GF256_TRY_AVX2)
    if (CpuHasAVX2)
    {
        GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<GF256_M256 *>(x16);
        const GF256_M256 * GF256_RESTRICT y32 = reinterpret_cast<const GF256_M256 *>(y16);

        while (bytes >= 128)
        {
            GF256_M256 x0 = _mm256_loadu_si256(x32);
            GF256_M256 y0 = _mm256_loadu_si256(y32);
            x0 = _mm256_xor_si256(x0, y0);
            GF256_M256 x1 = _mm256_loadu_si256(x32 + 1);
            GF256_M256 y1 = _mm256_loadu_si256(y32 + 1);
            x1 = _mm256_xor_si256(x1, y1);
            GF256_M256 x2 = _mm256_loadu_si256(x32 + 2);
            GF256_M256 y2 = _mm256_loadu_si256(y32 + 2);
            x2 = _mm256_xor_si256(x2, y2);
            GF256_M256 x3 = _mm256_loadu_si256(x32 + 3);
            GF256_M256 y3 = _mm256_loadu_si256(y32 + 3);
            x3 = _mm256_xor_si256(x3, y3);

            _mm256_storeu_si256(x32, x0);
            _mm256_storeu_si256(x32 + 1, x1);
            _mm256_storeu_si256(x32 + 2, x2);
            _mm256_storeu_si256(x32 + 3, x3);

            bytes -= 128, x32 += 4, y32 += 4;
        }

        // Handle multiples of 32 bytes
        while (bytes >= 32)
        {
            // x[i] = x[i] xor y[i]
            _mm256_storeu_si256(x32,
                _mm256_xor_si256(
                    _mm256_loadu_si256(x32),
                    _mm256_loadu_si256(y32)));

            bytes -= 32, ++x32, ++y32;
        }

        x16 = reinterpret_cast<GF256_M128 *>(x32);
        y16 = reinterpret_cast<const GF256_M128 *>(y32);
    }
    else
# endif // GF256_TRY_AVX2
    {
        while (bytes >= 64)
        {
            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 y0 = _mm_loadu_si128(y16);
            x0 = _mm_xor_si128(x0, y0);
            GF256_M128 x1 = _mm_loadu_si128(x16 + 1);
            GF256_M128 y1 = _mm_loadu_si128(y16 + 1);
            x1 = _mm_xor_si128(x1, y1);
            GF256_M128 x2 = _mm_loadu_si128(x16 + 2);
            GF256_M128 y2 = _mm_loadu_si128(y16 + 2);
            x2 = _mm_xor_si128(x2, y2);
            GF256_M128 x3 = _mm_loadu_si128(x16 + 3);
            GF256_M128 y3 = _mm_loadu_si128(y16 + 3);
            x3 = _mm_xor_si128(x3, y3);

            _mm_storeu_si128(x16, x0);
            _mm_storeu_si128(x16 + 1, x1);
            _mm_storeu_si128(x16 + 2, x2);
            _mm_storeu_si128(x16 + 3, x3);

            bytes -= 64, x16 += 4, y16 += 4;
        }
    }
#endif // GF256_TARGET_MOBILE

#if !defined(GF256_TARGET_MOBILE)
    // Handle multiples of 16 bytes
    while (bytes >= 16)
    {
        // x[i] = x[i] xor y[i]
        _mm_storeu_si128(x16,
            _mm_xor_si128(
                _mm_loadu_si128(x16),
                _mm_loadu_si128(y16)));

        bytes -= 16, ++x16, ++y16;
    }
#endif

    uint8_t * GF256_RESTRICT x1 = reinterpret_cast<uint8_t *>(x16);
    const uint8_t * GF256_RESTRICT y1 = reinterpret_cast<const uint8_t *>(y16);

    // Handle a block of 8 bytes
    const int eight = bytes & 8;
    if (eight)
    {
        uint64_t * GF256_RESTRICT x8 = reinterpret_cast<uint64_t *>(x1);
        const uint64_t * GF256_RESTRICT y8 = reinterpret_cast<const uint64_t *>(y1);
        *x8 ^= *y8;
    }

    // Handle a block of 4 bytes
    const int four = bytes & 4;
    if (four)
    {
        uint32_t * GF256_RESTRICT x4 = reinterpret_cast<uint32_t *>(x1 + eight);
        const uint32_t * GF256_RESTRICT y4 = reinterpret_cast<const uint32_t *>(y1 + eight);
        *x4 ^= *y4;
    }

    // Handle final bytes
    const int offset = eight + four;
    switch (bytes & 3)
    {
    case 3: x1[offset + 2] ^= y1[offset + 2];
    case 2: x1[offset + 1] ^= y1[offset + 1];
    case 1: x1[offset] ^= y1[offset];
    default:
        break;
    }
}

extern "C" void gf256_add2_mem(void * GF256_RESTRICT vz, const void * GF256_RESTRICT vx,
                               const void * GF256_RESTRICT vy, int bytes)
{
    GF256_M128 * GF256_RESTRICT z16 = reinterpret_cast<GF256_M128*>(vz);
    const GF256_M128 * GF256_RESTRICT x16 = reinterpret_cast<const GF256_M128*>(vx);
    const GF256_M128 * GF256_RESTRICT y16 = reinterpret_cast<const GF256_M128*>(vy);

#if defined(GF256_TARGET_MOBILE)
# if defined(GF256_TRY_NEON)
    // Handle multiples of 64 bytes
    if (CpuHasNeon)
    {
        // Handle multiples of 16 bytes
        while (bytes >= 16)
        {
            // z[i] = z[i] xor x[i] xor y[i]
            vst1q_u8((uint8_t*)z16,
                veorq_u8(
                    vld1q_u8((uint8_t*)z16),
                    veorq_u8(
                        vld1q_u8((uint8_t*)x16),
                        vld1q_u8((uint8_t*)y16))));

            bytes -= 16, ++x16, ++y16, ++z16;
        }
    }
    else
# endif // GF256_TRY_NEON
    {
        uint64_t * GF256_RESTRICT z8 = reinterpret_cast<uint64_t *>(z16);
        const uint64_t * GF256_RESTRICT x8 = reinterpret_cast<const uint64_t *>(x16);
        const uint64_t * GF256_RESTRICT y8 = reinterpret_cast<const uint64_t *>(y16);

        const unsigned count = (unsigned)bytes / 8;
        for (unsigned ii = 0; ii < count; ++ii)
            z8[ii] ^= x8[ii] ^ y8[ii];

        z16 = reinterpret_cast<GF256_M128 *>(z8 + count);
        x16 = reinterpret_cast<const GF256_M128 *>(x8 + count);
        y16 = reinterpret_cast<const GF256_M128 *>(y8 + count);

        bytes -= (count * 8);
    }
#else // GF256_TARGET_MOBILE
# if defined(GF256_TRY_AVX2)
    if (CpuHasAVX2)
    {
        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(z16);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(x16);
        const GF256_M256 * GF256_RESTRICT y32 = reinterpret_cast<const GF256_M256 *>(y16);

        const unsigned count = bytes / 32;
        for (unsigned i = 0; i < count; ++i)
        {
            _mm256_storeu_si256(z32 + i,
                _mm256_xor_si256(
                    _mm256_loadu_si256(z32 + i),
                    _mm256_xor_si256(
                        _mm256_loadu_si256(x32 + i),
                        _mm256_loadu_si256(y32 + i))));
        }

        bytes -= count * 32;
        z16 = reinterpret_cast<GF256_M128 *>(z32 + count);
        x16 = reinterpret_cast<const GF256_M128 *>(x32 + count);
        y16 = reinterpret_cast<const GF256_M128 *>(y32 + count);
    }
# endif // GF256_TRY_AVX2

    // Handle multiples of 16 bytes
    while (bytes >= 16)
    {
        // z[i] = z[i] xor x[i] xor y[i]
        _mm_storeu_si128(z16,
            _mm_xor_si128(
                _mm_loadu_si128(z16),
                _mm_xor_si128(
                    _mm_loadu_si128(x16),
                    _mm_loadu_si128(y16))));

        bytes -= 16, ++x16, ++y16, ++z16;
    }
#endif // GF256_TARGET_MOBILE

    uint8_t * GF256_RESTRICT z1 = reinterpret_cast<uint8_t *>(z16);
    const uint8_t * GF256_RESTRICT x1 = reinterpret_cast<const uint8_t *>(x16);
    const uint8_t * GF256_RESTRICT y1 = reinterpret_cast<const uint8_t *>(y16);

    // Handle a block of 8 bytes
    const int eight = bytes & 8;
    if (eight)
    {
        uint64_t * GF256_RESTRICT z8 = reinterpret_cast<uint64_t *>(z1);
        const uint64_t * GF256_RESTRICT x8 = reinterpret_cast<const uint64_t *>(x1);
        const uint64_t * GF256_RESTRICT y8 = reinterpret_cast<const uint64_t *>(y1);
        *z8 ^= *x8 ^ *y8;
    }

    // Handle a block of 4 bytes
    const int four = bytes & 4;
    if (four)
    {
        uint32_t * GF256_RESTRICT z4 = reinterpret_cast<uint32_t *>(z1 + eight);
        const uint32_t * GF256_RESTRICT x4 = reinterpret_cast<const uint32_t *>(x1 + eight);
        const uint32_t * GF256_RESTRICT y4 = reinterpret_cast<const uint32_t *>(y1 + eight);
        *z4 ^= *x4 ^ *y4;
    }

    // Handle final bytes
    const int offset = eight + four;
    switch (bytes & 3)
    {
    case 3: z1[offset + 2] ^= x1[offset + 2] ^ y1[offset + 2];
    case 2: z1[offset + 1] ^= x1[offset + 1] ^ y1[offset + 1];
    case 1: z1[offset] ^= x1[offset] ^ y1[offset];
    default:
        break;
    }
}

extern "C" void gf256_addset_mem(void * GF256_RESTRICT vz, const void * GF256_RESTRICT vx,
                                 const void * GF256_RESTRICT vy, int bytes)
{
    GF256_M128 * GF256_RESTRICT z16 = reinterpret_cast<GF256_M128*>(vz);
    const GF256_M128 * GF256_RESTRICT x16 = reinterpret_cast<const GF256_M128*>(vx);
    const GF256_M128 * GF256_RESTRICT y16 = reinterpret_cast<const GF256_M128*>(vy);

#if defined(GF256_TARGET_MOBILE)
# if defined(GF256_TRY_NEON)
    // Handle multiples of 64 bytes
    if (CpuHasNeon)
    {
        while (bytes >= 64)
        {
            GF256_M128 x0 = vld1q_u8((uint8_t*)x16);
            GF256_M128 x1 = vld1q_u8((uint8_t*)(x16 + 1));
            GF256_M128 x2 = vld1q_u8((uint8_t*)(x16 + 2));
            GF256_M128 x3 = vld1q_u8((uint8_t*)(x16 + 3));
            GF256_M128 y0 = vld1q_u8((uint8_t*)(y16));
            GF256_M128 y1 = vld1q_u8((uint8_t*)(y16 + 1));
            GF256_M128 y2 = vld1q_u8((uint8_t*)(y16 + 2));
            GF256_M128 y3 = vld1q_u8((uint8_t*)(y16 + 3));

            vst1q_u8((uint8_t*)z16,     veorq_u8(x0, y0));
            vst1q_u8((uint8_t*)(z16 + 1), veorq_u8(x1, y1));
            vst1q_u8((uint8_t*)(z16 + 2), veorq_u8(x2, y2));
            vst1q_u8((uint8_t*)(z16 + 3), veorq_u8(x3, y3));

            bytes -= 64, x16 += 4, y16 += 4, z16 += 4;
        }

        // Handle multiples of 16 bytes
        while (bytes >= 16)
        {
            // z[i] = x[i] xor y[i]
            vst1q_u8((uint8_t*)z16,
                     veorq_u8(
                         vld1q_u8((uint8_t*)x16),
                         vld1q_u8((uint8_t*)y16)));

            bytes -= 16, ++x16, ++y16, ++z16;
        }
    }
    else
# endif // GF256_TRY_NEON
    {
        uint64_t * GF256_RESTRICT z8 = reinterpret_cast<uint64_t *>(z16);
        const uint64_t * GF256_RESTRICT x8 = reinterpret_cast<const uint64_t *>(x16);
        const uint64_t * GF256_RESTRICT y8 = reinterpret_cast<const uint64_t *>(y16);

        const unsigned count = (unsigned)bytes / 8;
        for (unsigned ii = 0; ii < count; ++ii)
            z8[ii] = x8[ii] ^ y8[ii];

        x16 = reinterpret_cast<const GF256_M128 *>(x8 + count);
        y16 = reinterpret_cast<const GF256_M128 *>(y8 + count);
        z16 = reinterpret_cast<GF256_M128 *>(z8 + count);

        bytes -= (count * 8);
    }
#else // GF256_TARGET_MOBILE
# if defined(GF256_TRY_AVX2)
    if (CpuHasAVX2)
    {
        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(z16);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(x16);
        const GF256_M256 * GF256_RESTRICT y32 = reinterpret_cast<const GF256_M256 *>(y16);

        const unsigned count = bytes / 32;
        for (unsigned i = 0; i < count; ++i)
        {
            _mm256_storeu_si256(z32 + i,
                _mm256_xor_si256(
                    _mm256_loadu_si256(x32 + i),
                    _mm256_loadu_si256(y32 + i)));
        }

        bytes -= count * 32;
        z16 = reinterpret_cast<GF256_M128 *>(z32 + count);
        x16 = reinterpret_cast<const GF256_M128 *>(x32 + count);
        y16 = reinterpret_cast<const GF256_M128 *>(y32 + count);
    }
    else
# endif // GF256_TRY_AVX2
    {
        // Handle multiples of 64 bytes
        while (bytes >= 64)
        {
            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 x1 = _mm_loadu_si128(x16 + 1);
            GF256_M128 x2 = _mm_loadu_si128(x16 + 2);
            GF256_M128 x3 = _mm_loadu_si128(x16 + 3);
            GF256_M128 y0 = _mm_loadu_si128(y16);
            GF256_M128 y1 = _mm_loadu_si128(y16 + 1);
            GF256_M128 y2 = _mm_loadu_si128(y16 + 2);
            GF256_M128 y3 = _mm_loadu_si128(y16 + 3);

            _mm_storeu_si128(z16,     _mm_xor_si128(x0, y0));
            _mm_storeu_si128(z16 + 1, _mm_xor_si128(x1, y1));
            _mm_storeu_si128(z16 + 2, _mm_xor_si128(x2, y2));
            _mm_storeu_si128(z16 + 3, _mm_xor_si128(x3, y3));

            bytes -= 64, x16 += 4, y16 += 4, z16 += 4;
        }
    }

    // Handle multiples of 16 bytes
    while (bytes >= 16)
    {
        // z[i] = x[i] xor y[i]
        _mm_storeu_si128(z16,
            _mm_xor_si128(
                _mm_loadu_si128(x16),
                _mm_loadu_si128(y16)));

        bytes -= 16, ++x16, ++y16, ++z16;
    }
#endif // GF256_TARGET_MOBILE

    uint8_t * GF256_RESTRICT z1 = reinterpret_cast<uint8_t *>(z16);
    const uint8_t * GF256_RESTRICT x1 = reinterpret_cast<const uint8_t *>(x16);
    const uint8_t * GF256_RESTRICT y1 = reinterpret_cast<const uint8_t *>(y16);

    // Handle a block of 8 bytes
    const int eight = bytes & 8;
    if (eight)
    {
        uint64_t * GF256_RESTRICT z8 = reinterpret_cast<uint64_t *>(z1);
        const uint64_t * GF256_RESTRICT x8 = reinterpret_cast<const uint64_t *>(x1);
        const uint64_t * GF256_RESTRICT y8 = reinterpret_cast<const uint64_t *>(y1);
        *z8 = *x8 ^ *y8;
    }

    // Handle a block of 4 bytes
    const int four = bytes & 4;
    if (four)
    {
        uint32_t * GF256_RESTRICT z4 = reinterpret_cast<uint32_t *>(z1 + eight);
        const uint32_t * GF256_RESTRICT x4 = reinterpret_cast<const uint32_t *>(x1 + eight);
        const uint32_t * GF256_RESTRICT y4 = reinterpret_cast<const uint32_t *>(y1 + eight);
        *z4 = *x4 ^ *y4;
    }

    // Handle final bytes
    const int offset = eight + four;
    switch (bytes & 3)
    {
    case 3: z1[offset + 2] = x1[offset + 2] ^ y1[offset + 2];
    case 2: z1[offset + 1] = x1[offset + 1] ^ y1[offset + 1];
    case 1: z1[offset] = x1[offset] ^ y1[offset];
    default:
        break;
    }
}

extern "C" void gf256_mul_mem(void * GF256_RESTRICT vz, const void * GF256_RESTRICT vx, uint8_t y, int bytes)
{
    // Use a single if-statement to handle special cases
    if (y <= 1)
    {
        if (y == 0)
            memset(vz, 0, bytes);
        else if (vz != vx)
            memcpy(vz, vx, bytes);
        return;
    }

    GF256_M128 * GF256_RESTRICT z16 = reinterpret_cast<GF256_M128 *>(vz);
    const GF256_M128 * GF256_RESTRICT x16 = reinterpret_cast<const GF256_M128 *>(vx);

#if defined(GF256_TARGET_MOBILE)
#if defined(GF256_TRY_NEON)
    if (bytes >= 16 && CpuHasNeon)
    {
        // Partial product tables; see above
        const GF256_M128 table_lo_y = vld1q_u8((uint8_t*)(GF256Ctx.MM128.TABLE_LO_Y + y));
        const GF256_M128 table_hi_y = vld1q_u8((uint8_t*)(GF256Ctx.MM128.TABLE_HI_Y + y));

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M128 clr_mask = vdupq_n_u8(0x0f);

        // Handle multiples of 16 bytes
        do
        {
            // See above comments for details
            GF256_M128 x0 = vld1q_u8((uint8_t*)x16);
            GF256_M128 l0 = vandq_u8(x0, clr_mask);
            x0 = vshrq_n_u8(x0, 4);
            GF256_M128 h0 = vandq_u8(x0, clr_mask);
            l0 = vqtbl1q_u8(table_lo_y, l0);
            h0 = vqtbl1q_u8(table_hi_y, h0);
            vst1q_u8((uint8_t*)z16, veorq_u8(l0, h0));

            bytes -= 16, ++x16, ++z16;
        } while (bytes >= 16);
    }
#endif
#else
# if defined(GF256_TRY_AVX2)
    if (bytes >= 32 && CpuHasAVX2)
    {
        // Partial product tables; see above
        const GF256_M256 table_lo_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_LO_Y + y);
        const GF256_M256 table_hi_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M256 clr_mask = _mm256_set1_epi8(0x0f);

        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(vz);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(vx);

        // Handle multiples of 32 bytes
        do
        {
            // See above comments for details
            GF256_M256 x0 = _mm256_loadu_si256(x32);
            GF256_M256 l0 = _mm256_and_si256(x0, clr_mask);
            x0 = _mm256_srli_epi64(x0, 4);
            GF256_M256 h0 = _mm256_and_si256(x0, clr_mask);
            l0 = _mm256_shuffle_epi8(table_lo_y, l0);
            h0 = _mm256_shuffle_epi8(table_hi_y, h0);
            _mm256_storeu_si256(z32, _mm256_xor_si256(l0, h0));

            bytes -= 32, ++x32, ++z32;
        } while (bytes >= 32);

        z16 = reinterpret_cast<GF256_M128 *>(z32);
        x16 = reinterpret_cast<const GF256_M128 *>(x32);
    }
# endif // GF256_TRY_AVX2
    if (bytes >= 16 && CpuHasSSSE3)
    {
        // Partial product tables; see above
        const GF256_M128 table_lo_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_LO_Y + y);
        const GF256_M128 table_hi_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M128 clr_mask = _mm_set1_epi8(0x0f);

        // Handle multiples of 16 bytes
        do
        {
            // See above comments for details
            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 l0 = _mm_and_si128(x0, clr_mask);
            x0 = _mm_srli_epi64(x0, 4);
            GF256_M128 h0 = _mm_and_si128(x0, clr_mask);
            l0 = _mm_shuffle_epi8(table_lo_y, l0);
            h0 = _mm_shuffle_epi8(table_hi_y, h0);
            _mm_storeu_si128(z16, _mm_xor_si128(l0, h0));

            bytes -= 16, ++x16, ++z16;
        } while (bytes >= 16);
    }
#endif

    uint8_t * GF256_RESTRICT z1 = reinterpret_cast<uint8_t*>(z16);
    const uint8_t * GF256_RESTRICT x1 = reinterpret_cast<const uint8_t*>(x16);
    const uint8_t * GF256_RESTRICT table = GF256Ctx.GF256_MUL_TABLE + ((unsigned)y << 8);

    // Handle blocks of 8 bytes
    while (bytes >= 8)
    {
        uint64_t * GF256_RESTRICT z8 = reinterpret_cast<uint64_t *>(z1);
#ifdef GF256_IS_BIG_ENDIAN
        uint64_t word = (uint64_t)table[x1[0]] << 56;
        word |= (uint64_t)table[x1[1]] << 48;
        word |= (uint64_t)table[x1[2]] << 40;
        word |= (uint64_t)table[x1[3]] << 32;
        word |= (uint64_t)table[x1[4]] << 24;
        word |= (uint64_t)table[x1[5]] << 16;
        word |= (uint64_t)table[x1[6]] << 8;
        word |= (uint64_t)table[x1[7]];
#else
        uint64_t word = table[x1[0]];
        word |= (uint64_t)table[x1[1]] << 8;
        word |= (uint64_t)table[x1[2]] << 16;
        word |= (uint64_t)table[x1[3]] << 24;
        word |= (uint64_t)table[x1[4]] << 32;
        word |= (uint64_t)table[x1[5]] << 40;
        word |= (uint64_t)table[x1[6]] << 48;
        word |= (uint64_t)table[x1[7]] << 56;
#endif
        *z8 = word;

        bytes -= 8, x1 += 8, z1 += 8;
    }

    // Handle a block of 4 bytes
    const int four = bytes & 4;
    if (four)
    {
        uint32_t * GF256_RESTRICT z4 = reinterpret_cast<uint32_t *>(z1);
#ifdef GF256_IS_BIG_ENDIAN
        uint32_t word = (uint32_t)table[x1[0]] << 24;
        word |= (uint32_t)table[x1[1]] << 16;
        word |= (uint32_t)table[x1[2]] << 8;
        word |= (uint32_t)table[x1[3]];
#else
        uint32_t word = table[x1[0]];
        word |= (uint32_t)table[x1[1]] << 8;
        word |= (uint32_t)table[x1[2]] << 16;
        word |= (uint32_t)table[x1[3]] << 24;
#endif
        *z4 = word;
    }

    // Handle single bytes
    const int offset = four;
    switch (bytes & 3)
    {
    case 3: z1[offset + 2] = table[x1[offset + 2]];
    case 2: z1[offset + 1] = table[x1[offset + 1]];
    case 1: z1[offset] = table[x1[offset]];
    default:
        break;
    }
}

extern "C" void gf256_muladd_mem(void * GF256_RESTRICT vz, uint8_t y,
                                 const void * GF256_RESTRICT vx, int bytes)
{
    // Use a single if-statement to handle special cases
    if (y <= 1)
    {
        if (y == 1)
            gf256_add_mem(vz, vx, bytes);
        return;
    }

    GF256_M128 * GF256_RESTRICT z16 = reinterpret_cast<GF256_M128 *>(vz);
    const GF256_M128 * GF256_RESTRICT x16 = reinterpret_cast<const GF256_M128 *>(vx);

#if defined(GF256_TARGET_MOBILE)
#if defined(GF256_TRY_NEON)
    if (bytes >= 16 && CpuHasNeon)
    {
        // Partial product tables; see above
        const GF256_M128 table_lo_y = vld1q_u8((uint8_t*)(GF256Ctx.MM128.TABLE_LO_Y + y));
        const GF256_M128 table_hi_y = vld1q_u8((uint8_t*)(GF256Ctx.MM128.TABLE_HI_Y + y));

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M128 clr_mask = vdupq_n_u8(0x0f);

        // Handle multiples of 16 bytes
        do
        {
            // See above comments for details
            GF256_M128 x0 = vld1q_u8((uint8_t*)x16);
            GF256_M128 l0 = vandq_u8(x0, clr_mask);

            // x0 = vshrq_n_u8(x0, 4);
            x0 = (GF256_M128)vshrq_n_u64( (uint64x2_t)x0, 4);
            GF256_M128 h0 = vandq_u8(x0, clr_mask);
            l0 = vqtbl1q_u8(table_lo_y, l0);
            h0 = vqtbl1q_u8(table_hi_y, h0);
            const GF256_M128 p0 = veorq_u8(l0, h0);
            const GF256_M128 z0 = vld1q_u8((uint8_t*)z16);
            vst1q_u8((uint8_t*)z16, veorq_u8(p0, z0));
            bytes -= 16, ++x16, ++z16;
        } while (bytes >= 16);
    }
#endif
#else // GF256_TARGET_MOBILE
# if defined(GF256_TRY_AVX2)
    if (bytes >= 32 && CpuHasAVX2)
    {
        // Partial product tables; see above
        const GF256_M256 table_lo_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_LO_Y + y);
        const GF256_M256 table_hi_y = _mm256_loadu_si256(GF256Ctx.MM256.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M256 clr_mask = _mm256_set1_epi8(0x0f);

        GF256_M256 * GF256_RESTRICT z32 = reinterpret_cast<GF256_M256 *>(z16);
        const GF256_M256 * GF256_RESTRICT x32 = reinterpret_cast<const GF256_M256 *>(x16);

        // On my Reed Solomon codec, the encoder unit test runs in 640 usec without and 550 usec with the optimization (86% of the original time)
        const unsigned count = bytes / 64;
        for (unsigned i = 0; i < count; ++i)
        {
            // See above comments for details
            GF256_M256 x0 = _mm256_loadu_si256(x32 + i * 2);
            GF256_M256 l0 = _mm256_and_si256(x0, clr_mask);
            x0 = _mm256_srli_epi64(x0, 4);
            const GF256_M256 z0 = _mm256_loadu_si256(z32 + i * 2);
            GF256_M256 h0 = _mm256_and_si256(x0, clr_mask);
            l0 = _mm256_shuffle_epi8(table_lo_y, l0);
            h0 = _mm256_shuffle_epi8(table_hi_y, h0);
            const GF256_M256 p0 = _mm256_xor_si256(l0, h0);
            _mm256_storeu_si256(z32 + i * 2, _mm256_xor_si256(p0, z0));

            GF256_M256 x1 = _mm256_loadu_si256(x32 + i * 2 + 1);
            GF256_M256 l1 = _mm256_and_si256(x1, clr_mask);
            x1 = _mm256_srli_epi64(x1, 4);
            const GF256_M256 z1 = _mm256_loadu_si256(z32 + i * 2 + 1);
            GF256_M256 h1 = _mm256_and_si256(x1, clr_mask);
            l1 = _mm256_shuffle_epi8(table_lo_y, l1);
            h1 = _mm256_shuffle_epi8(table_hi_y, h1);
            const GF256_M256 p1 = _mm256_xor_si256(l1, h1);
            _mm256_storeu_si256(z32 + i * 2 + 1, _mm256_xor_si256(p1, z1));
        }
        bytes -= count * 64;
        z32 += count * 2;
        x32 += count * 2;

        if (bytes >= 32)
        {
            GF256_M256 x0 = _mm256_loadu_si256(x32);
            GF256_M256 l0 = _mm256_and_si256(x0, clr_mask);
            x0 = _mm256_srli_epi64(x0, 4);
            GF256_M256 h0 = _mm256_and_si256(x0, clr_mask);
            l0 = _mm256_shuffle_epi8(table_lo_y, l0);
            h0 = _mm256_shuffle_epi8(table_hi_y, h0);
            const GF256_M256 p0 = _mm256_xor_si256(l0, h0);
            const GF256_M256 z0 = _mm256_loadu_si256(z32);
            _mm256_storeu_si256(z32, _mm256_xor_si256(p0, z0));

            bytes -= 32;
            z32++;
            x32++;
        }

        z16 = reinterpret_cast<GF256_M128 *>(z32);
        x16 = reinterpret_cast<const GF256_M128 *>(x32);
    }
# endif // GF256_TRY_AVX2
    if (bytes >= 16 && CpuHasSSSE3)
    {
        // Partial product tables; see above
        const GF256_M128 table_lo_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_LO_Y + y);
        const GF256_M128 table_hi_y = _mm_loadu_si128(GF256Ctx.MM128.TABLE_HI_Y + y);

        // clr_mask = 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
        const GF256_M128 clr_mask = _mm_set1_epi8(0x0f);

        // This unroll seems to provide about 7% speed boost when AVX2 is disabled
        while (bytes >= 32)
        {
            bytes -= 32;

            GF256_M128 x1 = _mm_loadu_si128(x16 + 1);
            GF256_M128 l1 = _mm_and_si128(x1, clr_mask);
            x1 = _mm_srli_epi64(x1, 4);
            GF256_M128 h1 = _mm_and_si128(x1, clr_mask);
            l1 = _mm_shuffle_epi8(table_lo_y, l1);
            h1 = _mm_shuffle_epi8(table_hi_y, h1);
            const GF256_M128 z1 = _mm_loadu_si128(z16 + 1);

            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 l0 = _mm_and_si128(x0, clr_mask);
            x0 = _mm_srli_epi64(x0, 4);
            GF256_M128 h0 = _mm_and_si128(x0, clr_mask);
            l0 = _mm_shuffle_epi8(table_lo_y, l0);
            h0 = _mm_shuffle_epi8(table_hi_y, h0);
            const GF256_M128 z0 = _mm_loadu_si128(z16);

            const GF256_M128 p1 = _mm_xor_si128(l1, h1);
            _mm_storeu_si128(z16 + 1, _mm_xor_si128(p1, z1));

            const GF256_M128 p0 = _mm_xor_si128(l0, h0);
            _mm_storeu_si128(z16, _mm_xor_si128(p0, z0));

            x16 += 2, z16 += 2;
        }

        // Handle multiples of 16 bytes
        while (bytes >= 16)
        {
            // See above comments for details
            GF256_M128 x0 = _mm_loadu_si128(x16);
            GF256_M128 l0 = _mm_and_si128(x0, clr_mask);
            x0 = _mm_srli_epi64(x0, 4);
            GF256_M128 h0 = _mm_and_si128(x0, clr_mask);
            l0 = _mm_shuffle_epi8(table_lo_y, l0);
            h0 = _mm_shuffle_epi8(table_hi_y, h0);
            const GF256_M128 p0 = _mm_xor_si128(l0, h0);
            const GF256_M128 z0 = _mm_loadu_si128(z16);
            _mm_storeu_si128(z16, _mm_xor_si128(p0, z0));

            bytes -= 16, ++x16, ++z16;
        }
    }
#endif // GF256_TARGET_MOBILE

    uint8_t * GF256_RESTRICT z1 = reinterpret_cast<uint8_t*>(z16);
    const uint8_t * GF256_RESTRICT x1 = reinterpret_cast<const uint8_t*>(x16);
    const uint8_t * GF256_RESTRICT table = GF256Ctx.GF256_MUL_TABLE + ((unsigned)y << 8);

    // Handle blocks of 8 bytes
    while (bytes >= 8)
    {
        uint64_t * GF256_RESTRICT z8 = reinterpret_cast<uint64_t *>(z1);
#ifdef GF256_IS_BIG_ENDIAN
        uint64_t word = (uint64_t)table[x1[0]] << 56;
        word |= (uint64_t)table[x1[1]] << 48;
        word |= (uint64_t)table[x1[2]] << 40;
        word |= (uint64_t)table[x1[3]] << 32;
        word |= (uint64_t)table[x1[4]] << 24;
        word |= (uint64_t)table[x1[5]] << 16;
        word |= (uint64_t)table[x1[6]] << 8;
        word |= (uint64_t)table[x1[7]];
#else
        uint64_t word = table[x1[0]];
        word |= (uint64_t)table[x1[1]] << 8;
        word |= (uint64_t)table[x1[2]] << 16;
        word |= (uint64_t)table[x1[3]] << 24;
        word |= (uint64_t)table[x1[4]] << 32;
        word |= (uint64_t)table[x1[5]] << 40;
        word |= (uint64_t)table[x1[6]] << 48;
        word |= (uint64_t)table[x1[7]] << 56;
#endif
        *z8 ^= word;

        bytes -= 8, x1 += 8, z1 += 8;
    }

    // Handle a block of 4 bytes
    const int four = bytes & 4;
    if (four)
    {
        uint32_t * GF256_RESTRICT z4 = reinterpret_cast<uint32_t *>(z1);
#ifdef GF256_IS_BIG_ENDIAN
        uint32_t word = (uint32_t)table[x1[0]] << 24;
        word |= (uint32_t)table[x1[1]] << 16;
        word |= (uint32_t)table[x1[2]] << 8;
        word |= (uint32_t)table[x1[3]];
#else
        uint32_t word = table[x1[0]];
        word |= (uint32_t)table[x1[1]] << 8;
        word |= (uint32_t)table[x1[2]] << 16;
        word |= (uint32_t)table[x1[3]] << 24;
#endif
        *z4 ^= word;
    }

    // Handle single bytes
    const int offset = four;
    switch (bytes & 3)
    {
    case 3: z1[offset + 2] ^= table[x1[offset + 2]];
    case 2: z1[offset + 1] ^= table[x1[offset + 1]];
    case 1: z1[offset] ^= table[x1[offset]];
    default:
        break;
    }
}

extern "C" void gf256_memswap(void * GF256_RESTRICT vx, void * GF256_RESTRICT vy, int bytes)
{
#if defined(GF256_TARGET_MOBILE)
    uint64_t * GF256_RESTRICT x16 = reinterpret_cast<uint64_t *>(vx);
    uint64_t * GF256_RESTRICT y16 = reinterpret_cast<uint64_t *>(vy);

    const unsigned count = (unsigned)bytes / 8;
    for (unsigned ii = 0; ii < count; ++ii)
    {
        const uint64_t temp = x16[ii];
        x16[ii] = y16[ii];
        y16[ii] = temp;
    }

    x16 += count;
    y16 += count;
    bytes -= count * 8;
#else
    GF256_M128 * GF256_RESTRICT x16 = reinterpret_cast<GF256_M128 *>(vx);
    GF256_M128 * GF256_RESTRICT y16 = reinterpret_cast<GF256_M128 *>(vy);

    // Handle blocks of 16 bytes
    while (bytes >= 16)
    {
        GF256_M128 x0 = _mm_loadu_si128(x16);
        GF256_M128 y0 = _mm_loadu_si128(y16);
        _mm_storeu_si128(x16, y0);
        _mm_storeu_si128(y16, x0);

        bytes -= 16, ++x16, ++y16;
    }
#endif

    uint8_t * GF256_RESTRICT x1 = reinterpret_cast<uint8_t *>(x16);
    uint8_t * GF256_RESTRICT y1 = reinterpret_cast<uint8_t *>(y16);

    // Handle a block of 8 bytes
    const int eight = bytes & 8;
    if (eight)
    {
        uint64_t * GF256_RESTRICT x8 = reinterpret_cast<uint64_t *>(x1);
        uint64_t * GF256_RESTRICT y8 = reinterpret_cast<uint64_t *>(y1);

        uint64_t temp = *x8;
        *x8 = *y8;
        *y8 = temp;
    }

    // Handle a block of 4 bytes
    const int four = bytes & 4;
    if (four)
    {
        uint32_t * GF256_RESTRICT x4 = reinterpret_cast<uint32_t *>(x1 + eight);
        uint32_t * GF256_RESTRICT y4 = reinterpret_cast<uint32_t *>(y1 + eight);

        uint32_t temp = *x4;
        *x4 = *y4;
        *y4 = temp;
    }

    // Handle final bytes
    const int offset = eight + four;
    uint8_t temp;
    switch (bytes & 3)
    {
    case 3: temp = x1[offset + 2]; x1[offset + 2] = y1[offset + 2]; y1[offset + 2] = temp;
    case 2: temp = x1[offset + 1]; x1[offset + 1] = y1[offset + 1]; y1[offset + 1] = temp;
    case 1: temp = x1[offset]; x1[offset] = y1[offset]; y1[offset] = temp;
    default:
        break;
    }
}

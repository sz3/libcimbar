/*
 * libpopcnt.h - C/C++ library for counting the number of 1 bits (bit
 * population count) in an array as quickly as possible using
 * specialized CPU instructions i.e. POPCNT, AVX2, AVX512, NEON.
 *
 * Copyright (c) 2016 - 2024, Kim Walisch
 * Copyright (c) 2016 - 2018, Wojciech Muła
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBPOPCNT_H
#define LIBPOPCNT_H

#include <stdint.h>

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif

#ifndef __has_include
  #define __has_include(x) 0
#endif

#ifdef __GNUC__
  #define LIBPOPCNT_GNUC_PREREQ(x, y) \
      (__GNUC__ > x || (__GNUC__ == x && __GNUC_MINOR__ >= y))
#else
  #define LIBPOPCNT_GNUC_PREREQ(x, y) 0
#endif

#ifdef __clang__
  #define LIBPOPCNT_CLANG_PREREQ(x, y) \
      (__clang_major__ > x || (__clang_major__ == x && __clang_minor__ >= y))
#else
  #define LIBPOPCNT_CLANG_PREREQ(x, y) 0
#endif

#if (_MSC_VER < 1900) && \
    !defined(__cplusplus)
  #define inline __inline
#endif

#if (defined(__i386__) || \
     defined(__x86_64__) || \
     defined(_M_IX86) || \
     defined(_M_X64))
  #define LIBPOPCNT_X86_OR_X64
#endif

#if LIBPOPCNT_GNUC_PREREQ(4, 2) || \
    __has_builtin(__builtin_popcount)
  #define LIBPOPCNT_HAVE_BUILTIN_POPCOUNT
#endif

#if LIBPOPCNT_GNUC_PREREQ(4, 2) || \
    LIBPOPCNT_CLANG_PREREQ(3, 0)
  #define LIBPOPCNT_HAVE_ASM_POPCNT
#endif

#if defined(LIBPOPCNT_X86_OR_X64) && \
   (defined(LIBPOPCNT_HAVE_ASM_POPCNT) || \
    defined(_MSC_VER))
  #define LIBPOPCNT_HAVE_POPCNT
#endif

/* GCC compiler */
#if defined(LIBPOPCNT_X86_OR_X64) && \
    LIBPOPCNT_GNUC_PREREQ(5, 0)
  #define LIBPOPCNT_HAVE_AVX2
#endif

/* GCC compiler */
#if defined(LIBPOPCNT_X86_OR_X64) && \
    LIBPOPCNT_GNUC_PREREQ(11, 0)
  #define LIBPOPCNT_HAVE_AVX512
#endif

/* Clang (Unix-like OSes) */
#if defined(LIBPOPCNT_X86_OR_X64) && !defined(_MSC_VER)
  #if LIBPOPCNT_CLANG_PREREQ(3, 8) && \
      __has_attribute(target) && \
      (!defined(__apple_build_version__) || __apple_build_version__ >= 8000000)
    #define LIBPOPCNT_HAVE_AVX2
  #endif
  #if LIBPOPCNT_CLANG_PREREQ(9, 0) && \
      __has_attribute(target) && \
      (!defined(__apple_build_version__) || __apple_build_version__ >= 8000000)
    #define LIBPOPCNT_HAVE_AVX512
  #endif
#endif

/* MSVC compatible compilers (Windows) */
#if defined(LIBPOPCNT_X86_OR_X64) && \
    defined(_MSC_VER)
  /*
   * There is an LLVM/Clang bug on Windows where function targets
   * for AVX2 and AVX512 fail to compile unless the user compiles
   * using the options /arch:AVX2 and /arch:AVX512.
   * All Clang versions <= 18.0 (from 2024) are affected by this bug.
   * However, I expect this bug will be fixed in near future:
   * https://github.com/llvm/llvm-project/issues/53520
   */
  #if defined(__clang__)
    #if defined(__AVX2__)
      #define LIBPOPCNT_HAVE_AVX2
    #endif
    #if defined(__AVX512__)
      #define LIBPOPCNT_HAVE_AVX2
      #define LIBPOPCNT_HAVE_AVX512
    #endif
  /* MSVC 2017 or later does not require
  * /arch:AVX2 or /arch:AVX512 */
  #elif _MSC_VER >= 1910
    #define LIBPOPCNT_HAVE_AVX2
    #define LIBPOPCNT_HAVE_AVX512
  #endif
#endif

/*
 * Only enable CPUID runtime checks if this is really
 * needed. E.g. do not enable if user has compiled
 * using -march=native on a CPU that supports AVX512.
 */
#if defined(LIBPOPCNT_X86_OR_X64) && \
   (defined(__cplusplus) || \
    defined(_MSC_VER) || \
   (LIBPOPCNT_GNUC_PREREQ(4, 2) || \
    __has_builtin(__sync_val_compare_and_swap))) && \
   ((defined(LIBPOPCNT_HAVE_AVX512) && !(defined(__AVX512__) || \
                                        (defined(__AVX512F__) && \
                                         defined(__AVX512BW__) && \
                                         defined(__AVX512VPOPCNTDQ__)))) || \
    (defined(LIBPOPCNT_HAVE_AVX2) && !defined(__AVX2__)) || \
    (defined(LIBPOPCNT_HAVE_POPCNT) && !defined(__POPCNT__)))
  #define LIBPOPCNT_HAVE_CPUID
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This uses fewer arithmetic operations than any other known
 * implementation on machines with fast multiplication.
 * It uses 12 arithmetic operations, one of which is a multiply.
 * http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
 */
static inline uint64_t popcnt64_bitwise(uint64_t x)
{
  uint64_t m1 = 0x5555555555555555ull;
  uint64_t m2 = 0x3333333333333333ull;
  uint64_t m4 = 0x0F0F0F0F0F0F0F0Full;
  uint64_t h01 = 0x0101010101010101ull;

  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;

  return (x * h01) >> 56;
}

#if defined(LIBPOPCNT_HAVE_ASM_POPCNT) && \
    defined(__x86_64__)

static inline uint64_t popcnt64(uint64_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

#elif defined(LIBPOPCNT_HAVE_ASM_POPCNT) && \
      defined(__i386__)

static inline uint32_t popcnt32(uint32_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

static inline uint64_t popcnt64(uint64_t x)
{
  return popcnt32((uint32_t) x) +
         popcnt32((uint32_t)(x >> 32));
}

#elif defined(_MSC_VER) && \
      defined(_M_X64)

#include <intrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return __popcnt64(x);
}

#elif defined(_MSC_VER) && \
      defined(_M_IX86)

#include <intrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return __popcnt((uint32_t) x) + 
         __popcnt((uint32_t)(x >> 32));
}

/* non x86 CPUs */
#elif defined(LIBPOPCNT_HAVE_BUILTIN_POPCOUNT)

static inline uint64_t popcnt64(uint64_t x)
{
  return __builtin_popcountll(x);
}

/* no hardware POPCNT,
 * use pure integer algorithm */
#else

static inline uint64_t popcnt64(uint64_t x)
{
  return popcnt64_bitwise(x);
}

#endif

#if defined(LIBPOPCNT_HAVE_CPUID)

#if defined(_MSC_VER)
  #include <intrin.h>
  #include <immintrin.h>
#endif

/* CPUID bits documentation: */
/* https://en.wikipedia.org/wiki/CPUID */

/* %ebx bit flags */
#define LIBPOPCNT_BIT_AVX2     (1 << 5)
#define LIBPOPCNT_BIT_AVX512F  (1 << 16)
#define LIBPOPCNT_BIT_AVX512BW (1 << 30)

/* %ecx bit flags */
#define LIBPOPCNT_BIT_AVX512_VPOPCNTDQ (1 << 14)
#define LIBPOPCNT_BIT_POPCNT           (1 << 23)

/* xgetbv bit flags */
#define LIBPOPCNT_XSTATE_SSE (1 << 1)
#define LIBPOPCNT_XSTATE_YMM (1 << 2)
#define LIBPOPCNT_XSTATE_ZMM (7 << 5)

static inline void run_cpuid(int eax, int ecx, int* abcd)
{
#if defined(_MSC_VER)
  __cpuidex(abcd, eax, ecx);
#else
  int ebx = 0;
  int edx = 0;

  #if defined(__i386__) && \
      defined(__PIC__)
    /* In case of PIC under 32-bit EBX cannot be clobbered */
    __asm__ __volatile__("movl %%ebx, %%edi;"
                         "cpuid;"
                         "xchgl %%ebx, %%edi;"
                         : "+a" (eax),
                           "=D" (ebx),
                           "+c" (ecx),
                           "=d" (edx));
  #else
    __asm__ __volatile__("cpuid"
                         : "+a" (eax),
                           "+b" (ebx),
                           "+c" (ecx),
                           "=d" (edx));
  #endif

  abcd[0] = eax;
  abcd[1] = ebx;
  abcd[2] = ecx;
  abcd[3] = edx;
#endif
}

#if defined(LIBPOPCNT_HAVE_AVX2) || \
    defined(LIBPOPCNT_HAVE_AVX512)

static inline uint64_t get_xcr0(void)
{
#if defined(_MSC_VER)
  return _xgetbv(0);
#else
  uint32_t eax;
  uint32_t edx;

  __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
  return eax | (((uint64_t) edx) << 32);
#endif
}

#endif

static inline int get_cpuid(void)
{
  int flags = 0;
  int abcd[4];

  run_cpuid(1, 0, abcd);

  if ((abcd[2] & LIBPOPCNT_BIT_POPCNT) == LIBPOPCNT_BIT_POPCNT)
    flags |= LIBPOPCNT_BIT_POPCNT;

#if defined(LIBPOPCNT_HAVE_AVX2) || \
    defined(LIBPOPCNT_HAVE_AVX512)

  int osxsave_mask = (1 << 27);

  /* ensure OS supports extended processor state management */
  if ((abcd[2] & osxsave_mask) != osxsave_mask)
    return 0;

  uint64_t ymm_mask = LIBPOPCNT_XSTATE_SSE | LIBPOPCNT_XSTATE_YMM;
  uint64_t zmm_mask = LIBPOPCNT_XSTATE_SSE | LIBPOPCNT_XSTATE_YMM | LIBPOPCNT_XSTATE_ZMM;
  uint64_t xcr0 = get_xcr0();

  if ((xcr0 & ymm_mask) == ymm_mask)
  {
    run_cpuid(7, 0, abcd);

    if ((abcd[1] & LIBPOPCNT_BIT_AVX2) == LIBPOPCNT_BIT_AVX2)
      flags |= LIBPOPCNT_BIT_AVX2;

    if ((xcr0 & zmm_mask) == zmm_mask)
    {
      /* If all AVX512 features required by our popcnt_avx512() are supported */
      /* then we add LIBPOPCNT_BIT_AVX512_VPOPCNTDQ to our CPUID flags. */
      if ((abcd[1] & LIBPOPCNT_BIT_AVX512F) == LIBPOPCNT_BIT_AVX512F &&
          (abcd[1] & LIBPOPCNT_BIT_AVX512BW) == LIBPOPCNT_BIT_AVX512BW &&
          (abcd[2] & LIBPOPCNT_BIT_AVX512_VPOPCNTDQ) == LIBPOPCNT_BIT_AVX512_VPOPCNTDQ)
        flags |= LIBPOPCNT_BIT_AVX512_VPOPCNTDQ;
    }
  }

#endif

  return flags;
}

#endif /* cpuid */

#if defined(LIBPOPCNT_HAVE_AVX2) && \
    __has_include(<immintrin.h>)

#include <immintrin.h>

#if __has_attribute(target)
  __attribute__ ((target ("avx2")))
#endif
static inline void CSA256(__m256i* h, __m256i* l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = _mm256_xor_si256(a, b);
  *h = _mm256_or_si256(_mm256_and_si256(a, b), _mm256_and_si256(u, c));
  *l = _mm256_xor_si256(u, c);
}

#if __has_attribute(target)
  __attribute__ ((target ("avx2")))
#endif
static inline __m256i popcnt256(__m256i v)
{
  __m256i lookup1 = _mm256_setr_epi8(
      4, 5, 5, 6, 5, 6, 6, 7,
      5, 6, 6, 7, 6, 7, 7, 8,
      4, 5, 5, 6, 5, 6, 6, 7,
      5, 6, 6, 7, 6, 7, 7, 8
  );

  __m256i lookup2 = _mm256_setr_epi8(
      4, 3, 3, 2, 3, 2, 2, 1,
      3, 2, 2, 1, 2, 1, 1, 0,
      4, 3, 3, 2, 3, 2, 2, 1,
      3, 2, 2, 1, 2, 1, 1, 0
  );

  __m256i low_mask = _mm256_set1_epi8(0x0f);
  __m256i lo = _mm256_and_si256(v, low_mask);
  __m256i hi = _mm256_and_si256(_mm256_srli_epi16(v, 4), low_mask);
  __m256i popcnt1 = _mm256_shuffle_epi8(lookup1, lo);
  __m256i popcnt2 = _mm256_shuffle_epi8(lookup2, hi);

  return _mm256_sad_epu8(popcnt1, popcnt2);
}

/*
 * AVX2 Harley-Seal popcount (4th iteration).
 * The algorithm is based on the paper "Faster Population Counts
 * using AVX2 Instructions" by Daniel Lemire, Nathan Kurz and
 * Wojciech Mula (23 Nov 2016).
 * @see https://arxiv.org/abs/1611.07612
 */
#if __has_attribute(target)
  __attribute__ ((target ("avx2")))
#endif
static inline uint64_t popcnt_avx2(const __m256i* ptr, uint64_t size)
{
  __m256i cnt = _mm256_setzero_si256();
  __m256i ones = _mm256_setzero_si256();
  __m256i twos = _mm256_setzero_si256();
  __m256i fours = _mm256_setzero_si256();
  __m256i eights = _mm256_setzero_si256();
  __m256i sixteens = _mm256_setzero_si256();
  __m256i twosA, twosB, foursA, foursB, eightsA, eightsB;

  uint64_t i = 0;
  uint64_t limit = size - size % 16;
  uint64_t* cnt64;

  for(; i < limit; i += 16)
  {
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 0), _mm256_loadu_si256(ptr + i + 1));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 2), _mm256_loadu_si256(ptr + i + 3));
    CSA256(&foursA, &twos, twos, twosA, twosB);
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 4), _mm256_loadu_si256(ptr + i + 5));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 6), _mm256_loadu_si256(ptr + i + 7));
    CSA256(&foursB, &twos, twos, twosA, twosB);
    CSA256(&eightsA, &fours, fours, foursA, foursB);
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 8), _mm256_loadu_si256(ptr + i + 9));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 10), _mm256_loadu_si256(ptr + i + 11));
    CSA256(&foursA, &twos, twos, twosA, twosB);
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 12), _mm256_loadu_si256(ptr + i + 13));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 14), _mm256_loadu_si256(ptr + i + 15));
    CSA256(&foursB, &twos, twos, twosA, twosB);
    CSA256(&eightsB, &fours, fours, foursA, foursB);
    CSA256(&sixteens, &eights, eights, eightsA, eightsB);

    cnt = _mm256_add_epi64(cnt, popcnt256(sixteens));
  }

  cnt = _mm256_slli_epi64(cnt, 4);
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(eights), 3));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(fours), 2));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(twos), 1));
  cnt = _mm256_add_epi64(cnt, popcnt256(ones));

  for(; i < size; i++)
    cnt = _mm256_add_epi64(cnt, popcnt256(_mm256_loadu_si256(ptr + i)));

  cnt64 = (uint64_t*) &cnt;

  return cnt64[0] +
         cnt64[1] +
         cnt64[2] +
         cnt64[3];
}

#endif

#if defined(LIBPOPCNT_HAVE_AVX512) && \
    __has_include(<immintrin.h>)

#include <immintrin.h>

#if __has_attribute(target)
  __attribute__ ((target ("avx512f,avx512bw,avx512vpopcntdq")))
#endif
static inline uint64_t popcnt_avx512(const uint8_t* ptr8, uint64_t size)
{
    __m512i cnt = _mm512_setzero_si512();
    const uint64_t* ptr64 = (const uint64_t*) ptr8;
    uint64_t size64 = size / sizeof(uint64_t);
    uint64_t i = 0;

    for (; i + 32 <= size64; i += 32)
    {
      __m512i vec0 = _mm512_loadu_epi64(&ptr64[i + 0]);
      __m512i vec1 = _mm512_loadu_epi64(&ptr64[i + 8]);
      __m512i vec2 = _mm512_loadu_epi64(&ptr64[i + 16]);
      __m512i vec3 = _mm512_loadu_epi64(&ptr64[i + 24]);

      vec0 = _mm512_popcnt_epi64(vec0);
      vec1 = _mm512_popcnt_epi64(vec1);
      vec2 = _mm512_popcnt_epi64(vec2);
      vec3 = _mm512_popcnt_epi64(vec3);

      cnt = _mm512_add_epi64(cnt, vec0);
      cnt = _mm512_add_epi64(cnt, vec1);
      cnt = _mm512_add_epi64(cnt, vec2);
      cnt = _mm512_add_epi64(cnt, vec3);
    }

    for (; i + 8 <= size64; i += 8)
    {
      __m512i vec = _mm512_loadu_epi64(&ptr64[i]);
      vec = _mm512_popcnt_epi64(vec);
      cnt = _mm512_add_epi64(cnt, vec);
    }

    i *= sizeof(uint64_t);

    /* Process last 64 bytes */
    if (i < size)
    {
      __mmask64 mask = (__mmask64) (0xffffffffffffffffull >> (i + 64 - size));
      __m512i vec = _mm512_maskz_loadu_epi8(mask, &ptr8[i]);
      vec = _mm512_popcnt_epi64(vec);
      cnt = _mm512_add_epi64(cnt, vec);
    }

    return _mm512_reduce_add_epi64(cnt);
}

#endif

/* x86 CPUs */
#if defined(LIBPOPCNT_X86_OR_X64)

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static uint64_t popcnt(const void* data, uint64_t size)
{
/*
 * CPUID runtime checks are only enabled if this is needed.
 * E.g. CPUID is disabled when a user compiles his
 * code using -march=native on a CPU with AVX512.
 */
#if defined(LIBPOPCNT_HAVE_CPUID)
  #if defined(__cplusplus)
    /* C++11 thread-safe singleton */
    static const int cpuid = get_cpuid();
  #else
    static int cpuid_ = -1;
    int cpuid = cpuid_;
    if (cpuid == -1)
    {
      cpuid = get_cpuid();

      #if defined(_MSC_VER)
        _InterlockedCompareExchange(&cpuid_, cpuid, -1);
      #else
        __sync_val_compare_and_swap(&cpuid_, -1, cpuid);
      #endif
    }
  #endif
#endif

  const uint8_t* ptr = (const uint8_t*) data;
  uint64_t cnt = 0;
  uint64_t i = 0;

#if defined(LIBPOPCNT_HAVE_AVX512)
  #if defined(__AVX512__) || \
     (defined(__AVX512F__) && \
      defined(__AVX512BW__) && \
      defined(__AVX512VPOPCNTDQ__))
    /* For tiny arrays AVX512 is not worth it */
    if (i + 40 <= size)
  #else
    if ((cpuid & LIBPOPCNT_BIT_AVX512_VPOPCNTDQ) &&
        i + 40 <= size)
  #endif
      return popcnt_avx512(ptr, size);
#endif

#if defined(LIBPOPCNT_HAVE_AVX2)
  #if defined(__AVX2__)
    /* AVX2 requires arrays >= 512 bytes */
    if (i + 512 <= size)
  #else
    if ((cpuid & LIBPOPCNT_BIT_AVX2) &&
        i + 512 <= size)
  #endif
    {
      const __m256i* ptr256 = (const __m256i*)(ptr + i);
      cnt += popcnt_avx2(ptr256, (size - i) / 32);
      i = size - size % 32;
    }
#endif

#if defined(LIBPOPCNT_HAVE_POPCNT)
  /* 
   * The user has compiled without -mpopcnt.
   * Unfortunately the MSVC compiler does not have
   * a POPCNT macro so we cannot get rid of the
   * runtime check for MSVC.
   */
  #if !defined(__POPCNT__)
    if (cpuid & LIBPOPCNT_BIT_POPCNT)
  #endif
    {
      if (i + 8 <= size)
      {
        uintptr_t rem = ((uintptr_t) &ptr[i]) % 8;

        /* Align &ptr[i] to an 8 byte boundary */
        if (rem != 0)
        {
          uint64_t val = 0;
          uint64_t bytes = (uint64_t) (8 - rem % 8);
          bytes = (bytes <= 7) ? bytes : 7;
          for (uint64_t j = 0; j < bytes; j++)
            val |= ((uint64_t) ptr[i + j]) << (j * 8);
          cnt += popcnt64(val);
          i += bytes;
        }
      }

      for (; i + 8 <= size; i += 8)
        cnt += popcnt64(*(const uint64_t*)(ptr + i));

      if (i < size)
      {
        uint64_t val = 0;
        uint64_t bytes = (uint64_t) (size - i);
        bytes = (bytes <= 7) ? bytes : 7;
        for (uint64_t j = 0; j < bytes; j++)
          val |= ((uint64_t) ptr[i + j]) << (j * 8);
        cnt += popcnt64(val);
      }

      return cnt;
    }
#endif

/*
 * This code is used for:
 * 1) Compiler does not support POPCNT.
 * 2) x86 CPU does not support POPCNT (cpuid != POPCNT).
 */
#if !defined(LIBPOPCNT_HAVE_POPCNT) || \
    !defined(__POPCNT__)

  if (i + 8 <= size)
  {
    uintptr_t rem = ((uintptr_t) &ptr[i]) % 8;

    /* Align &ptr[i] to an 8 byte boundary */
    if (rem != 0)
    {
      uint64_t val = 0;
      uint64_t bytes = (uint64_t) (8 - rem % 8);
      bytes = (bytes <= 7) ? bytes : 7;
      for (uint64_t j = 0; j < bytes; j++)
        val |= ((uint64_t) ptr[i + j]) << (j * 8);
      cnt += popcnt64_bitwise(val);
      i += bytes;
    }
  }

  for (; i + 8 <= size; i += 8)
    cnt += popcnt64_bitwise(*(const uint64_t*)(ptr + i));

  if (i < size)
  {
    uint64_t val = 0;
    uint64_t bytes = (uint64_t) (size - i);
    bytes = (bytes <= 7) ? bytes : 7;
    for (uint64_t j = 0; j < bytes; j++)
      val |= ((uint64_t) ptr[i + j]) << (j * 8);
    cnt += popcnt64_bitwise(val);
  }

  return cnt;
#endif
}

/* Compile with e.g. -march=armv8-a+sve to enable ARM SVE */
#elif defined(__ARM_FEATURE_SVE) && \
      __has_include(<arm_sve.h>)

#include <arm_sve.h>

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i = 0;
  const uint64_t* ptr64 = (const uint64_t*) data;
  uint64_t size64 = size / sizeof(uint64_t);
  svuint64_t vcnt = svdup_u64(0);

  for (; i + svcntd() * 4 <= size64; i += svcntd() * 4)
  {
    svuint64_t vec0 = svld1_u64(svptrue_b64(), &ptr64[i + svcntd() * 0]);
    svuint64_t vec1 = svld1_u64(svptrue_b64(), &ptr64[i + svcntd() * 1]);
    svuint64_t vec2 = svld1_u64(svptrue_b64(), &ptr64[i + svcntd() * 2]);
    svuint64_t vec3 = svld1_u64(svptrue_b64(), &ptr64[i + svcntd() * 3]);

    vec0 = svcnt_u64_x(svptrue_b64(), vec0);
    vec1 = svcnt_u64_x(svptrue_b64(), vec1);
    vec2 = svcnt_u64_x(svptrue_b64(), vec2);
    vec3 = svcnt_u64_x(svptrue_b64(), vec3);

    vcnt = svadd_u64_x(svptrue_b64(), vcnt, vec0);
    vcnt = svadd_u64_x(svptrue_b64(), vcnt, vec1);
    vcnt = svadd_u64_x(svptrue_b64(), vcnt, vec2);
    vcnt = svadd_u64_x(svptrue_b64(), vcnt, vec3);
  }

  svbool_t pg = svwhilelt_b64(i, size64);

  while (svptest_any(svptrue_b64(), pg))
  {
    svuint64_t vec = svld1_u64(pg, &ptr64[i]);
    vec = svcnt_u64_z(pg, vec);
    vcnt = svadd_u64_x(svptrue_b64(), vcnt, vec);
    i += svcntd();
    pg = svwhilelt_b64(i, size64);
  }

  uint64_t cnt = svaddv_u64(svptrue_b64(), vcnt);
  uint64_t bytes = size % sizeof(uint64_t);

  if (bytes != 0)
  {
    i = size - bytes;
    const uint8_t* ptr8 = (const uint8_t*) data;
    svbool_t pg8 = svwhilelt_b8(i, size);
    svuint8_t vec = svld1_u8(pg8, &ptr8[i]);
    svuint8_t vcnt8 = svcnt_u8_z(pg8, vec);
    cnt += svaddv_u8(pg8, vcnt8);
  }

  return cnt;
}

#elif (defined(__ARM_NEON) || \
       defined(__aarch64__) || \
       defined(_M_ARM64)) && \
      __has_include(<arm_neon.h>)

#include <arm_neon.h>

static inline uint64x2_t vpadalq(uint64x2_t sum, uint8x16_t t)
{
  return vpadalq_u32(sum, vpaddlq_u16(vpaddlq_u8(t)));
}

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i = 0;
  uint64_t cnt = 0;
  uint64_t chunk_size = 64;
  const uint8_t* ptr = (const uint8_t*) data;

  if (size >= chunk_size)
  {
    uint64_t iters = size / chunk_size;
    uint64x2_t sum = vcombine_u64(vcreate_u64(0), vcreate_u64(0));
    uint8x16_t zero = vcombine_u8(vcreate_u8(0), vcreate_u8(0));

    do
    {
      uint8x16_t t0 = zero;
      uint8x16_t t1 = zero;
      uint8x16_t t2 = zero;
      uint8x16_t t3 = zero;

      /*
       * After every 31 iterations we need to add the
       * temporary sums (t0, t1, t2, t3) to the total sum.
       * We must ensure that the temporary sums <= 255
       * and 31 * 8 bits = 248 which is OK.
       */
      uint64_t limit = (i + 31 < iters) ? i + 31 : iters;
  
      /* Each iteration processes 64 bytes */
      for (; i < limit; i++)
      {
        uint8x16x4_t input = vld4q_u8(ptr);
        ptr += chunk_size;

        t0 = vaddq_u8(t0, vcntq_u8(input.val[0]));
        t1 = vaddq_u8(t1, vcntq_u8(input.val[1]));
        t2 = vaddq_u8(t2, vcntq_u8(input.val[2]));
        t3 = vaddq_u8(t3, vcntq_u8(input.val[3]));
      }

      sum = vpadalq(sum, t0);
      sum = vpadalq(sum, t1);
      sum = vpadalq(sum, t2);
      sum = vpadalq(sum, t3);
    }
    while (i < iters);

    i = 0;
    size %= chunk_size;

    uint64_t tmp[2];
    vst1q_u64(tmp, sum);
    cnt += tmp[0];
    cnt += tmp[1];
  }

  if (i + 8 <= size)
  {
    uintptr_t rem = ((uintptr_t) &ptr[i]) % 8;

    /* Align &ptr[i] to an 8 byte boundary */
    if (rem != 0)
    {
      uint64_t val = 0;
      uint64_t bytes = (uint64_t) (8 - rem % 8);
      bytes = (bytes <= 7) ? bytes : 7;
      for (uint64_t j = 0; j < bytes; j++)
        val |= ((uint64_t) ptr[i + j]) << (j * 8);
      cnt += popcnt64(val);
      i += bytes;
    }
  }

  for (; i + 8 <= size; i += 8)
    cnt += popcnt64(*(const uint64_t*)(ptr + i));

  if (i < size)
  {
    uint64_t val = 0;
    uint64_t bytes = (uint64_t) (size - i);
    bytes = (bytes <= 7) ? bytes : 7;
    for (uint64_t j = 0; j < bytes; j++)
      val |= ((uint64_t) ptr[i + j]) << (j * 8);
    cnt += popcnt64(val);
  }

  return cnt;
}

/* all other CPUs */
#else

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i = 0;
  uint64_t cnt = 0;
  const uint8_t* ptr = (const uint8_t*) data;

  if (i + 8 <= size)
  {
    uintptr_t rem = ((uintptr_t) &ptr[i]) % 8;

    /* Align &ptr[i] to an 8 byte boundary */
    if (rem != 0)
    {
      uint64_t val = 0;
      uint64_t bytes = (uint64_t) (8 - rem % 8);
      bytes = (bytes <= 7) ? bytes : 7;
      for (uint64_t j = 0; j < bytes; j++)
        val |= ((uint64_t) ptr[i + j]) << (j * 8);
      cnt += popcnt64(val);
      i += bytes;
    }
  }

  for (; i + 8 <= size; i += 8)
    cnt += popcnt64(*(const uint64_t*)(ptr + i));

  if (i < size)
  {
    uint64_t val = 0;
    uint64_t bytes = (uint64_t) (size - i);
    bytes = (bytes <= 7) ? bytes : 7;
    for (uint64_t j = 0; j < bytes; j++)
      val |= ((uint64_t) ptr[i + j]) << (j * 8);
    cnt += popcnt64(val);
  }

  return cnt;
}

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBPOPCNT_H */

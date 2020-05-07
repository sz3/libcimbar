/** \file
    \brief Wirehair : Tools
    \copyright Copyright (c) 2012-2018 Christopher A. Taylor.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of Wirehair nor the names of its contributors may be
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

#include "WirehairTools.h"

#include <cmath>

#ifdef _MSC_VER
#include <intrin.h> // _BitScanReverse
#pragma intrinsic(_BitScanReverse)
#endif


namespace wirehair {


//------------------------------------------------------------------------------
// Utility: 16-bit Integer Square Root function

static const uint8_t kSquareRootTable[256] = {
    0, 16, 16, 16, 32, 32, 32, 32, 32, 48, 48, 48, 48, 48, 48, 48,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96,
    96, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112, 112,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    143, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155, 156,
    157, 158, 159, 159, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168, 169,
    170, 170, 171, 172, 173, 173, 174, 175, 175, 176, 177, 178, 178, 179, 180, 181,
    181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188, 189, 189, 190, 191, 191,
    192, 193, 193, 194, 195, 195, 196, 197, 197, 198, 199, 199, 200, 201, 201, 202,
    203, 203, 204, 204, 205, 206, 206, 207, 207, 208, 209, 209, 210, 211, 211, 212,
    212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 219, 220, 221, 221,
    222, 222, 223, 223, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230, 230,
    231, 231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238, 239,
    239, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247,
    248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255, 255,
};

/*
    Based on code from http://www.azillionmonkeys.com/qed/sqroot.html

        "Contributors include Arne Steinarson for the basic approximation idea, 
        Dann Corbit and Mathew Hendry for the first cut at the algorithm, 
        Lawrence Kirby for the rearrangement, improvments and range optimization
        and Paul Hsieh for the round-then-adjust idea."

    I tried this out, stdlib sqrt() and a few variations on Newton-Raphson
    and determined this one is, by far, the fastest.  I adapted it to 16-bit
    input for additional performance and tweaked the operations to work best
    with the MSVC optimizer, which turned out to be very sensitive to the
    way that the code is written.

    My table generator produces a different table than from the website, but that
    is because I stop at 16-bit values.
*/

/// Returns first position with set bit (LSB = 0)
/// Precondition: x != 0
static inline unsigned Log32(uint32_t x)
{
#ifdef _MSC_VER
    unsigned long index;
    // Note: Ignoring result because x != 0
    _BitScanReverse(&index, x);
    return (unsigned)index;
#else
    // Note: Ignoring return value of 0 because x != 0
    return 31 - (unsigned)__builtin_clz(x);
#endif
}

// There is a pretty good discussion of porting CLZ here:
// https://embeddedgurus.com/state-space/tag/clz/

uint16_t FloorSquareRoot16(uint16_t x)
{
    unsigned nonzeroBits;
    if (x < 0x100) {
        nonzeroBits = 6;
    }
    else {
        // TBD: It might be faster on ARM to just use branches
        nonzeroBits = Log32(x);
    }

    // See TableGenerator.cpp for logic here
    const unsigned tableShift = (nonzeroBits - 6) & 0xfe;
    const unsigned resultShift = (15 - nonzeroBits) / 2;
    uint16_t r = kSquareRootTable[x >> tableShift] >> resultShift;

    // Correct rounding if necessary (compiler optimizes this form better)
    r -= (r * r > x);

    return r;
}


//------------------------------------------------------------------------------
// Utility: 16-bit Truncated Sieve of Eratosthenes Next Prime function

/// Size of the kSieveTable in elements
static const int kSieveTableSize = 2 * 3 * 5 * 7;

/// Sieve table for the next prime finding function below
static const uint8_t kSieveTable[kSieveTableSize] = {
    1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    7, 6, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
};

/// Number of primes under 256 and above 11
static const unsigned kPrimesUnder256Count = 50;

/// List of primes under 256 and above 11
static const uint8_t kPrimesUnder256From11[50] = {
    11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
    47, 53, 59, 61, 67, 71, 73, 79, 83, 89,
    97, 101, 103, 107, 109, 113, 127, 131, 137, 139,
    149, 151, 157, 163, 167, 173, 179, 181, 191, 193,
    197, 199, 211, 223, 227, 229, 233, 239, 241, 251,
};

uint16_t NextPrime16(uint16_t n)
{
    // Handle small n
    switch (n)
    {
    case 0:
        CAT_DEBUG_BREAK(); // Invalid input
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    case 4:
    case 5: return 5;
    case 6:
    case 7: return 7;
    }

    // Handle large n
    if (n > 65521) {
        CAT_DEBUG_BREAK(); // Invalid input
        return 0;
    }

    // Choose first n from table
    unsigned offset = n % kSieveTableSize;
    uint16_t next = kSieveTable[offset];
    offset += next + 1;
    n += next;

    // Initialize p_max to floor(sqrt(n))
    unsigned p_max = FloorSquareRoot16(n);
    unsigned p_max2 = p_max * p_max;
    const uint8_t * GF256_RESTRICT primes = &kPrimesUnder256From11[0];

    // For each number to try:
TryNext:

    // For each prime to test up to square root:
    for (unsigned i = 0; i < kPrimesUnder256Count; ++i)
    {
        // If the next prime is above p_max we are done!
        const uint8_t p = primes[i];
        if (p > p_max) {
            return n;
        }

        // If composite, try next n
        if (n % p == 0)
        {
            // Use table to choose next trial number
            if (offset >= kSieveTableSize) {
                offset -= kSieveTableSize;
            }

            // Get next to test
            next = kSieveTable[offset];
            offset += next + 1;
            n += next + 1;

            // Derivative square root iteration of p_max
            if (p_max2 < n)
            {
                ++p_max;
                p_max2 = p_max * p_max;
            }

            goto TryNext;
        }
    }

    return n;
}


//------------------------------------------------------------------------------
// Utility: GF(2) Invertible Matrix Generator function

/**
    Sometimes it is helpful to be able to quickly generate a GF2 matrix
    that is invertible.  I guess.  Anyway, here's a lookup table of
    seeds that create invertible GF2 matrices and a function that will
    fill a bitfield with the matrix.

    This is generated and unit tested in TableGenerator.cpp
*/

static const unsigned kMatrixSeedCount = 512;

static const uint8_t kInvertibleMatrixSeeds[kMatrixSeedCount] = {
    0,1,4,2,0,12,0,1,1,0,2,2,1,4,1,1,1,9,9,1,0,0,0,0,2,2,2,4,3,4,1,1,
    0,0,0,0,7,1,6,2,5,1,0,4,0,0,10,0,11,3,3,2,10,0,0,0,0,0,0,1,0,0,1,6,
    4,0,2,3,0,0,0,1,0,0,3,3,3,1,4,0,2,2,3,1,6,0,4,1,2,3,1,1,7,0,7,0,
    0,3,5,2,6,1,3,3,0,7,0,0,4,7,1,7,5,4,1,4,4,4,6,3,0,0,4,5,5,1,8,1,
    1,0,2,4,4,2,2,2,4,5,0,0,2,0,2,4,1,3,3,3,0,4,0,0,0,0,0,0,2,1,1,17,
    3,3,3,2,9,14,3,2,5,1,1,6,3,1,6,0,0,5,5,3,3,0,0,0,0,7,4,7,7,10,0,1,
    1,9,0,0,0,1,3,0,1,2,0,2,4,1,4,0,0,2,0,4,3,3,1,0,1,10,12,5,2,2,4,1,
    0,1,1,1,0,6,4,0,0,0,3,1,3,5,0,10,2,0,3,3,1,1,0,1,6,19,2,2,1,3,5,3,
    1,1,1,4,4,0,0,3,2,8,1,14,3,0,0,1,0,1,1,4,1,1,2,0,6,8,6,5,0,0,0,6,
    1,1,4,1,6,3,0,0,1,6,3,2,2,1,3,1,9,6,8,3,1,0,3,0,0,1,1,5,2,2,4,0,
    1,2,2,6,5,7,6,0,10,3,5,0,0,0,4,0,2,10,3,1,2,2,0,2,0,9,0,0,5,3,4,4,
    4,2,0,7,0,0,0,0,2,5,0,0,0,4,5,0,0,2,4,7,3,2,2,0,1,8,1,0,2,2,2,6,
    2,0,0,0,9,0,0,0,1,0,1,1,3,0,0,3,3,3,5,0,0,1,0,1,1,1,14,12,9,2,11,3,
    2,3,3,3,3,7,6,6,0,0,0,11,0,0,3,2,1,1,1,2,3,2,3,3,3,3,3,0,0,0,1,8,
    4,3,4,4,4,1,4,6,0,6,4,3,7,6,0,1,3,3,0,2,1,1,1,1,2,0,1,0,0,8,3,2,
    2,2,2,2,9,0,0,0,0,0,0,0,0,0,5,9,0,0,2,3,2,0,0,10,11,0,0,0,2,0,6,0,
};

static GF256_FORCE_INLINE uint64_t Random64(PCGRandom& prng)
{
    const uint32_t rv1 = prng.Next();
    const uint32_t rv2 = prng.Next();
    return ((uint64_t)rv2 << 32) | rv1;
}

void AddInvertibleGF2Matrix(
    uint64_t * GF256_RESTRICT matrix, ///< Address of upper left word
    const unsigned offset, ///< Offset in bits to the first sum column
    const unsigned pitchWords, ///< Pitch of the matrix in words
    const unsigned n ///< Number of bits in the matrix to generate
)
{
    // If we do not have this value of n in the table:
    if (n >= kMatrixSeedCount)
    {
        uint64_t * row = matrix;

        // For each row:
        for (unsigned ii = 0; ii < n; ++ii, row += pitchWords)
        {
            // Flip diagonal bit
            const unsigned column_i = offset + ii;
            row[column_i >> 6] ^= (uint64_t)1 << (column_i & 63);

            // Flip an off-diagonal bit also
            if (ii + 1 < n) {
                const unsigned column_j = column_i + 1;
                row[column_j >> 6] ^= (uint64_t)1 << (column_j & 63);
            }
        }

        return;
    }

    // Pull a random matrix out of the lookup table
    const uint8_t seed = kInvertibleMatrixSeeds[n];

    PCGRandom prng;
    prng.Seed(seed);

    const unsigned shift = offset & 63;
    uint64_t * row = matrix + (offset >> 6);

    // For each row to generate:
    for (unsigned row_i = 0; row_i < n; ++row_i, row += pitchWords)
    {
        uint64_t * dest = row;
        unsigned target = n;

        // Unroll the first word
        uint64_t prev = 0;

        // Generate first word
        const uint64_t word0 = Random64(prng);

        uint64_t temp0 = word0;

        unsigned written0 = 64 - shift;

        // Handle short writes
        if (target < 64) {
            temp0 &= ((uint64_t)1 << target) - 1;
            if (written0 > target) {
                written0 = target;
            }
        }

        // Add partial word into front
        *dest++ ^= temp0 << shift;

        target -= written0;
        prev = word0;

        if (target <= 0) {
            continue;
        }

        // For each remaining word:
        for (;;)
        {
            uint64_t temp = 0;
            if (shift != 0) {
                temp = prev >> (64 - shift);
            }
            uint64_t word = 0;

            // If it needs another word:
            if (target > shift) {
                // Generate next word
                word = Random64(prng);
                temp |= (word << shift);
            }

            if (target < 64) {
                temp &= ((uint64_t)1 << target) - 1;
            }

            // Add partial word
            *dest ^= temp;

            if (target <= 64) {
                break;
            }

            target -= 64;
            ++dest;
            prev = word;
        }
    }
}


//------------------------------------------------------------------------------
// Utility: Deck Shuffling function

void ShuffleDeck16(
    PCGRandom &prng,
    uint16_t * GF256_RESTRICT deck,
    const uint32_t count)
{
    deck[0] = 0;

    // If we can unroll 4 times:
    if (count <= 256)
    {
        for (uint32_t ii = 1;;)
        {
            uint32_t jj, rv = prng.Next();

            // 8-bit unroll
            switch (count - ii)
            {
            default:
                jj = (uint8_t)rv % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
                jj = (uint8_t)(rv >> 8) % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
                jj = (uint8_t)(rv >> 16) % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
                jj = (uint8_t)(rv >> 24) % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
                break;

            case 3:
                jj = (uint8_t)rv % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
            case 2:
                jj = (uint8_t)(rv >> 8) % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
            case 1:
                jj = (uint8_t)(rv >> 16) % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
            case 0:
                return;
            }
        }
    }
    else
    {
        // For each deck entry:
        for (uint32_t ii = 1;;)
        {
            uint32_t jj, rv = prng.Next();

            // 16-bit unroll
            switch (count - ii)
            {
            default:
                jj = (uint16_t)rv % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
                jj = (uint16_t)(rv >> 16) % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
                ++ii;
                break;

            case 1:
                jj = (uint16_t)rv % ii;
                deck[ii] = deck[jj];
                deck[jj] = (uint16_t)ii;
            case 0:
                return;
            }
        }
    }
}


//------------------------------------------------------------------------------
// Utility: Peeling Row Weight Generator function

/// Stop using weight-1 after this
static const uint32_t kMaxNForWeight1 = 2048;

/// Maximum columns per peel matrix row
static const unsigned kMaxPeelCount = 64;

/// Map from a 32-bit uniform random number to a row weight
/// Probability of weight i = (i-1)/i
/// e.g. P(2) = 1/2, P(3) = 2/3, ...
/// P(1) is chosen based on logic below.
/// P(64) is set to 1 to truncate the tail of the distribution
static const uint32_t kPeelCountDistribution[kMaxPeelCount] = {
    0x00000000, 0x7fffffff, 0xaaaaaaa9, 0xbfffffff, 0xcccccccb, 0xd5555554, 0xdb6db6da, 0xdfffffff,
    0xe38e38e2, 0xe6666665, 0xe8ba2e8a, 0xeaaaaaa9, 0xec4ec4eb, 0xedb6db6c, 0xeeeeeeed, 0xefffffff,
    0xf0f0f0ef, 0xf1c71c70, 0xf286bca0, 0xf3333332, 0xf3cf3cf2, 0xf45d1744, 0xf4de9bd2, 0xf5555554,
    0xf5c28f5b, 0xf6276275, 0xf684bda0, 0xf6db6db5, 0xf72c234e, 0xf7777776, 0xf7bdef7a, 0xf7ffffff,
    0xf83e0f82, 0xf8787877, 0xf8af8af7, 0xf8e38e37, 0xf914c1b9, 0xf9435e4f, 0xf96f96f8, 0xf9999998,
    0xf9c18f9b, 0xf9e79e78, 0xfa0be82e, 0xfa2e8ba1, 0xfa4fa4f9, 0xfa6f4de8, 0xfa8d9df4, 0xfaaaaaa9,
    0xfac687d5, 0xfae147ad, 0xfafafaf9, 0xfb13b13a, 0xfb2b78c0, 0xfb425ecf, 0xfb586fb4, 0xfb6db6da,
    0xfb823edf, 0xfb9611a6, 0xfba93867, 0xfbbbbbba, 0xfbcda3ab, 0xfbdef7bc, 0xfbefbefa, 0xffffffff,
};

// Select probability of weight-1 rows here:
// P = 1/128
static const uint32_t P1 = (uint32_t)((1. / 128) * 0xffffffff);

// Remaining probabilities are taken from table
static const uint32_t P2 = kPeelCountDistribution[1];
static const uint32_t P3 = kPeelCountDistribution[2];

uint16_t GeneratePeelRowWeight(
    uint32_t rv, ///< 32-bit random value
    uint16_t block_count ///< Number of input blocks
)
{
    // If peel column count is small:
    if (block_count <= kMaxNForWeight1)
    {
        if (rv < P1) {
            return 1;
        }

        // Rescale to match table values
        rv -= P1;
    }

    // Unroll first 3 for speed (common case):

    if (rv <= P2) {
        return 2;
    }

    if (rv <= P3) {
        return 3;
    }

    uint16_t weight = 3;

    // Find first table entry containing a number smaller than or equal to rv
    while (rv > kPeelCountDistribution[weight++])
    {
        CAT_DEBUG_ASSERT(weight < kMaxPeelCount);
    }

    return weight;
}


//------------------------------------------------------------------------------
// Utility: Peel Matrix Row Parameter Initialization

void PeelRowParameters::Initialize(
    uint32_t row_seed,
    uint32_t p_seed,
    uint16_t peel_column_count,
    uint16_t mix_column_count)
{
    // Initialize PRNG
    PCGRandom prng;
    prng.Seed(row_seed, p_seed);

    // Generate peeling matrix row weight
    const uint16_t weight = GeneratePeelRowWeight(prng.Next(), peel_column_count);

    // Do not set more than N/2 at a time
    const uint16_t max_weight = peel_column_count / 2;

    PeelCount = (weight > max_weight) ? max_weight : weight;

    CAT_DEBUG_ASSERT(PeelCount > 0 && PeelCount <= kMaxPeelCount);

    const uint32_t rv_peel = prng.Next();

    // Generate peeling matrix column selection parameters for row
    PeelAdd = ((uint16_t)rv_peel % (peel_column_count - 1)) + 1;
    PeelFirst = (uint16_t)(rv_peel >> 16) % peel_column_count;

    const uint32_t rv_mix = prng.Next();

    // Generate mixing matrix column selection parameters
    MixAdd = ((uint16_t)rv_mix % (mix_column_count - 1)) + 1;
    MixFirst = (uint16_t)(rv_mix >> 16) % mix_column_count;
}


//------------------------------------------------------------------------------
// SIMD-Safe Aligned Memory Allocations

uint8_t* SIMDSafeAllocate(size_t size)
{
    uint8_t* data = (uint8_t*)calloc(1, GF256_ALIGN_BYTES + size);
    if (!data) {
        return nullptr;
    }
    unsigned offset = (unsigned)((uintptr_t)data % GF256_ALIGN_BYTES);
    data += GF256_ALIGN_BYTES - offset;
    data[-1] = (uint8_t)offset;
    return data;
}

void SIMDSafeFree(void* ptr)
{
    if (!ptr) {
        return;
    }
    uint8_t* data = (uint8_t*)ptr;
    unsigned offset = data[-1];
    if (offset >= GF256_ALIGN_BYTES) {
        CAT_DEBUG_BREAK(); // Should never happen
        return;
    }
    data -= GF256_ALIGN_BYTES - offset;
    free(data);
}


//------------------------------------------------------------------------------
// Tables for small N

// These are generated and updated by GenerateSmallDenseSeeds.cpp

const uint8_t kTinyDenseCounts[kTinyTableCount] = {
    0,0,2,3,3,5,6,6,6,7,9,10,10,10,12,14,13,14,12,12,15,16,21,14,14,13,18,21,22,21,13,22,
    13,24,14,17,16,24,30,26,24,18,15,15,24,18,21,17,14,16,21,18,17,22,25,20,17,18,21,18,23,20,19,23,
    19
};

const uint16_t kTinyDenseSeeds[kTinyTableCount] = {
    0,0,12678,31247,24246,6830,10311,18975,46844,888,38598,63780,12170,24444,55961,13141,53472,55486,28496,27214,49117,60737,62051,59811,3052,33624,35793,8210,32674,36155,10819,52727,
    9692,26215,33106,34092,18779,61056,4502,7786,36397,53163,36617,16009,60476,31637,38086,32027,968,59896,27819,10193,64755,41508,44381,41571,52121,23594,313,38382,48549,44868,43789,8396,
    1902
};

// This table skips the first kTinyTableCount elements
const uint8_t kSmallDenseSeeds[kSmallTableCount] = {
    28,34,248,36,160,229,20,209,229,27,76,188,144,26,95,176,51,20,78,4,121,26,79,158,196,93,233,4,1,1,8,214,
    150,139,93,135,224,4,62,27,89,74,150,69,3,119,126,72,145,60,110,57,18,182,140,5,110,8,158,57,196,68,39,138,
    3,17,124,119,32,53,69,47,158,39,12,131,44,68,187,168,226,173,92,46,112,89,102,114,231,84,25,237,65,36,151,93,
    119,118,147,123,155,62,120,95,161,59,137,12,91,30,119,104,111,203,11,109,207,234,97,117,31,139,207,181,73,127,169,181,
    80,246,29,100,6,32,88,2,67,133,185,236,225,16,148,36,16,115,85,108,160,101,171,213,34,107,238,85,89,13,77,14,
    133,57,136,242,208,5,225,141,176,155,64,196,142,82,79,64,99,95,74,60,180,72,27,39,13,237,26,60,125,22,188,134,
    131,6,25,43,180,61,51,4,81,250,85,123,68,107,129,42,8,160,143,107,181,24,106,126,2,199,23,160,49,109,217,108,
    170,238,123,5,4,148,48,45,153,1,121,52,105,255,114,172,115,184,66,220,75,173,133,160,77,220,209,63,248,241,64,228,
    70,77,175,29,18,166,144,89,66,131,113,226,22,223,149,125,24,175,51,99,31,145,101,19,106,15,235,118,221,175,148,116,
    18,22,156,25,87,154,22,229,7,178,201,116,44,45,9,237,115,218,86,198,107,94,31,49,224,10,43,231,87,72,48,4,
    177,213,84,54,143,238,33,155,81,29,57,188,8,240,148,25,32,34,156,199,154,128,130,24,39,182,33,75,13,143,41,137,
    15,241,21,92,177,159,73,71,36,46,103,7,106,123,12,195,114,87,255,82,110,50,89,172,158,0,176,242,127,208,110,239,
    75,141,58,168,178,150,212,160,91,24,90,184,60,56,190,171,48,26,220,86,143,153,90,60,46,90,74,63,116,245,92,176,
    160,70,116,76,61,199,220,86,21,253,77,90,18,240,16,103,72,11,66,26,210,187,54,193,95,81,209,28,51,108,72,61,
    6,73,12,54,154,123,140,45,46,108,152,84,244,11,28,205,64,38,18,135,102,1,52,36,73,27,102,42,0,86,28,28,
    197,176,160,182,166,31,156,156,156,1,244,168,235,103,43,152,55,82,153,217,35,185,152,123,228,106,184,121,39,226,32,182,
    64,157,205,91,139,139,42,10,84,52,66,188,197,161,60,224,83,19,205,40,48,30,47,233,152,162,4,236,230,39,190,240,
    85,98,217,93,12,27,150,62,53,7,142,96,192,27,252,166,4,16,78,8,16,248,59,170,26,193,48,1,70,52,18,146,
    24,133,123,235,171,205,45,130,107,250,73,173,85,47,1,95,64,188,25,118,40,30,47,132,4,51,125,155,70,166,210,189,
    204,143,226,185,63,254,248,93,100,202,167,155,47,28,196,190,5,242,55,51,152,88,248,48,68,24,169,254,33,65,191,1,
    245,243,12,48,41,180,189,171,51,14,61,142,14,134,209,123,157,192,149,200,137,204,158,196,114,83,39,58,237,54,134,195,
    20,143,184,156,16,40,17,208,101,95,23,99,26,108,211,215,14,218,115,180,132,4,190,117,116,15,161,139,40,215,124,148,
    206,12,169,48,90,28,22,8,144,249,39,15,251,80,128,151,91,132,17,112,167,121,33,111,0,153,228,20,196,93,113,43,
    139,14,193,140,72,58,201,157,176,131,10,39,18,37,157,6,45,8,36,144,236,79,52,181,11,125,86,219,96,3,15,21,
    42,16,160,156,196,65,210,172,10,2,0,127,41,84,107,215,17,94,115,168,95,163,40,144,203,102,173,26,102,72,76,10,
    193,48,225,192,194,209,13,88,224,253,34,128,197,129,146,9,168,87,36,183,2,171,117,6,122,118,5,38,87,61,96,95,
    217,188,233,203,101,252,180,27,9,210,157,200,57,7,41,92,119,37,125,42,138,208,68,223,36,30,59,139,61,59,252,161,
    31,114,7,168,59,210,23,83,114,63,29,24,128,93,180,107,132,84,178,60,100,181,98,65,28,81,155,2,228,226,138,143,
    31,219,180,90,59,228,159,207,50,24,58,215,91,89,77,67,114,237,45,10,150,8,230,180,1,253,104,22,3,7,112,83,
    93,98,96,71,6,144,106,168,239,89,72,29,102,182,144,19,11,223,65,47,144,26,58,144,152,15,29,140,53,119,166,102,
    165,145,30,30,137,183,24,23,40,232,151,108,43,243,61,89,233,66,219,72,208,42,247,8,144,15,0,71,53,11,234,58,
    197,12,140,57,32,61,149,95,99,73,115,79,163,79,102,57,248,105,32,59,235,28,211,78,220,93,3,7,44,231,204,124,
    224,250,131,232,173,99,111,118,180,108,178,44,205,241,169,29,5,18,139,192,54,28,69,193,41,113,179,62,131,25,1,125,
    62,11,237,135,73,105,242,100,175,90,236,197,156,85,39,96,63,60,100,69,243,88,243,158,114,13,139,169,125,52,44,23,
    234,41,120,226,236,80,84,26,204,188,39,16,126,202,103,64,174,68,115,152,69,163,225,181,39,189,38,2,95,120,164,49,
    253,208,60,13,35,108,77,140,55,87,219,76,36,98,161,70,45,167,50,139,153,122,216,229,19,218,117,13,155,175,53,252,
    138,160,52,74,67,46,205,116,100,105,118,33,22,181,105,151,176,112,74,168,179,50,13,28,27,229,203,139,253,204,57,8,
    23,216,99,209,3,1,167,120,89,81,43,20,135,142,233,109,216,198,10,128,162,213,174,57,37,26,82,16,220,122,27,2,
    139,160,179,71,62,177,90,0,243,93,24,39,48,93,174,129,222,63,60,19,154,65,35,226,90,43,189,28,83,207,239,244,
    11,166,74,184,207,12,85,109,58,30,9,163,217,230,149,178,248,22,147,6,206,88,178,7,50,158,18,173,251,6,68,234,
    11,69,245,10,123,189,78,183,14,6,23,15,44,145,162,45,19,39,121,101,156,25,181,14,148,32,184,49,91,60,15,101,
    93,143,29,220,145,117,26,94,26,92,133,67,101,121,25,8,35,74,232,137,35,138,206,78,137,160,145,183,217,149,37,3,
    7,226,30,164,228,32,126,115,66,223,45,16,254,144,226,40,20,126,4,162,89,171,13,223,162,104,226,1,47,238,232,199,
    109,86,100,44,126,226,105,246,62,21,65,130,32,210,58,168,236,233,166,35,229,113,127,76,153,123,240,97,13,140,31,230,
    175,37,80,48,197,108,169,183,35,118,48,16,176,97,77,26,186,63,150,183,146,31,25,120,4,39,7,60,23,242,62,141,
    39,115,103,111,114,130,198,10,39,184,106,129,59,76,111,159,69,116,58,59,85,29,88,155,112,2,250,26,33,224,136,66,
    218,137,36,44,150,135,117,106,74,155,64,201,178,86,127,3,141,49,11,173,79,114,83,156,16,20,205,179,70,157,97,138,
    50,125,71,75,43,87,180,198,161,27,221,119,49,159,142,221,200,127,120,130,195,31,22,153,78,184,137,57,79,116,58,21,
    70,68,121,83,111,63,85,28,34,27,200,150,89,147,160,129,75,59,140,84,29,132,101,142,83,175,50,96,142,56,1,222,
    104,128,241,195,70,151,10,0,103,16,56,237,191,127,35,4,30,157,96,79,224,12,153,210,95,29,191,62,132,216,166,73,
    28,222,150,109,59,165,12,103,90,214,7,10,64,58,229,35,1,60,172,63,162,178,186,83,63,108,103,207,221,119,50,8,
    138,192,17,162,36,64,3,156,206,5,174,65,31,165,116,90,89,155,28,108,44,192,195,5,52,25,27,157,117,82,181,121,
    78,224,39,40,84,15,1,107,122,103,240,105,71,207,218,24,89,50,199,207,159,37,3,182,32,242,134,147,20,5,194,129,
    162,226,57,50,65,53,172,242,16,28,107,169,139,179,53,72,192,182,186,85,112,234,65,35,17,79,156,185,12,83,29,41,
    183,3,91,71,245,45,240,121,72,54,20,211,233,205,126,13,94,181,107,33,84,85,197,115,58,29,83,165,32,151,29,72,
    218,233,164,230,149,27,207,164,130,148,75,183,31,152,87,30,5,191,5,53,136,63,225,204,40,149,124,108,240,255,13,24,
    189,127,154,142,247,11,116,47,114,130,87,35,141,216,90,79,19,72,237,240,18,102,24,35,122,65,111,117,141,155,119,12,
    4,157,100,107,8,55,102,249,142,99,152,204,35,176,214,236,125,17,41,226,228,48,76,141,16,252,238,57,64,64,118,196,
    163,26,246,86,197,100,40,0,185,78,21,177,179,50,178,57,8,30,160,52,158,38,33,82,121,225,189,185,22,172,90,49,
    9,4,49,19,114,168,117,53,243,63,33,76,231,105,238,16,187,11,20,23,166,77,243,86,34,249,253,3,7,52,57,177,
    177,23,233,60,127,113,200,13,56,40,197,71,149,242,81,87,167,254,171,115,53,10,12,77,8,66,207,123,141,2,46,95,
    83,140,20,22,179,245,165,160,238,20,22,57,112,48,95,220,15,187,42,7,99,58,31,45,36,43,94,76,54,217,200
};

const uint8_t kSmallPeelSeeds[kTinyTableCount + kSmallTableCount] = {
    0,0,0,11,1,0,0,0,1,0,0,0,5,1,0,0,4,11,0,0,0,3,0,1,7,2,3,12,11,9,2,3,
    0,3,1,9,21,46,5,8,27,4,37,37,69,5,180,8,30,201,21,23,12,59,151,77,214,216,221,46,23,89,39,11,
    46,228,189,11,18,24,41,229,31,123,11,39,119,19,10,102,178,23,131,210,2,116,219,34,43,105,22,128,87,187,179,89,
    70,54,65,173,190,85,98,98,238,16,57,80,67,115,135,178,80,195,135,183,113,116,199,137,4,152,33,146,185,46,153,141,
    1,0,4,3,0,7,0,3,6,13,5,2,1,0,2,3,1,3,2,1,1,4,2,2,3,0,3,7,1,0,5,0,
    0,4,0,4,1,5,0,2,4,130,5,6,10,6,0,4,3,0,16,2,4,0,19,0,7,0,7,5,0,4,1,4,
    5,1,4,2,8,7,15,7,3,1,1,2,2,0,2,2,14,3,6,6,5,3,2,12,14,12,22,16,3,2,3,6,
    5,10,2,1,1,0,1,6,2,0,7,9,10,5,1,10,4,2,2,2,9,3,1,1,33,5,11,4,4,7,8,0,
    0,3,0,10,3,1,5,7,5,37,9,0,10,12,2,13,2,3,12,5,20,0,5,0,0,20,9,2,23,12,31,2,
    14,3,3,2,23,4,0,14,1,3,10,4,22,5,28,0,23,19,4,13,14,4,29,12,19,21,5,1,5,1,22,2,
    7,3,5,30,11,18,2,15,60,9,6,0,8,7,1,9,4,13,0,37,42,23,29,8,5,5,9,17,8,5,8,51,
    17,2,3,21,11,27,30,51,10,7,45,7,13,12,5,30,12,24,50,47,60,98,31,47,23,4,16,45,10,2,27,45,
    42,11,44,81,34,26,3,7,22,2,5,19,16,8,63,9,39,71,41,6,51,2,88,4,13,6,19,7,35,23,6,42,
    4,2,26,35,37,49,45,19,40,63,78,37,30,1,19,2,8,1,15,31,0,6,12,0,16,4,10,86,25,9,18,21,
    13,0,50,62,0,43,107,8,29,19,60,35,81,11,118,10,39,53,4,11,37,78,33,32,61,7,19,21,26,7,6,11,
    81,44,72,37,3,17,98,28,15,11,113,42,163,98,15,122,13,119,12,50,45,4,85,37,124,40,0,45,113,39,52,112,
    19,88,32,48,36,90,18,39,15,20,43,71,74,183,57,79,79,0,110,8,12,100,16,47,78,93,0,41,20,1,10,36,
    21,162,2,39,203,16,79,25,2,1,158,122,134,15,36,73,42,157,91,0,8,34,242,18,39,98,211,49,41,69,5,56,
    71,25,110,87,48,51,5,69,64,135,42,122,10,65,20,126,181,108,112,124,200,10,135,14,135,0,136,11,70,29,7,199,
    226,225,171,130,50,44,80,31,89,41,80,18,65,43,21,53,70,37,162,165,91,91,33,87,105,87,110,215,13,32,122,19,
    69,18,178,8,30,28,2,206,128,61,5,38,43,9,6,122,51,17,85,180,60,16,23,63,83,112,80,115,36,63,18,146,
    22,59,83,24,71,34,10,237,51,126,2,168,64,18,37,28,135,146,5,80,4,17,44,105,47,68,219,26,119,13,0,138,
    60,26,93,24,75,20,39,21,242,42,62,17,18,0,83,1,36,76,152,107,138,2,246,15,65,26,16,49,203,59,216,127,
    64,255,28,198,137,22,14,42,20,198,47,67,8,48,67,40,10,244,159,217,27,208,6,54,143,3,21,80,119,82,133,117,
    152,107,42,38,153,213,45,29,20,10,140,107,14,11,33,26,41,32,123,139,104,42,213,2,3,12,79,44,156,40,65,237,
    41,5,63,52,149,251,37,10,143,1,162,90,138,43,12,97,46,4,105,43,81,39,222,107,35,28,10,62,48,21,27,186,
    105,0,171,248,6,20,145,10,52,3,215,151,13,24,165,131,170,42,46,139,114,43,22,28,73,48,231,70,142,77,224,133,
    153,46,35,251,153,108,2,197,175,37,56,45,28,6,77,1,182,112,43,4,137,169,19,203,154,129,13,6,215,16,47,89,
    1,3,53,63,245,175,45,164,158,85,236,16,15,13,185,171,18,95,25,19,12,106,12,254,24,68,50,107,53,155,44,5,
    231,15,4,103,38,24,66,38,166,38,61,20,129,212,49,2,3,101,100,148,106,87,37,3,2,210,157,2,68,109,34,96,
    26,176,76,173,172,46,47,102,255,233,122,224,38,27,27,54,54,47,32,7,85,148,158,136,113,145,211,241,128,17,88,34,
    69,224,246,52,27,43,30,90,239,52,82,248,231,26,81,146,0,2,216,209,80,39,14,46,10,93,255,170,31,203,89,242,
    79,81,174,225,16,6,52,244,236,149,28,74,57,63,135,25,86,17,28,124,18,168,158,140,125,168,84,221,245,49,163,150,
    100,89,20,18,8,11,184,31,30,77,196,39,149,200,170,52,120,29,136,7,28,45,211,190,83,34,27,1,197,163,93,36,
    53,177,97,233,226,80,238,41,186,226,45,43,226,107,20,93,215,59,125,175,77,86,123,23,171,14,28,36,1,99,9,54,
    38,213,27,38,99,61,155,186,199,203,143,112,0,68,215,177,111,178,124,21,173,205,103,47,77,27,111,68,252,251,249,175,
    97,144,7,5,43,170,112,30,18,186,109,150,129,253,53,28,42,41,31,51,144,0,68,6,175,71,141,3,29,114,70,225,
    5,34,121,23,151,240,119,38,238,8,51,179,201,7,206,244,98,15,53,195,239,27,200,72,112,198,23,64,6,66,153,201,
    46,26,78,5,160,59,209,18,25,168,215,128,73,77,195,26,98,28,71,73,84,228,227,67,189,125,39,60,108,140,29,86,
    71,185,63,65,98,72,74,18,58,76,28,44,36,192,113,249,55,214,220,19,177,29,252,94,129,218,117,223,17,14,160,232,
    17,25,49,14,57,93,250,194,193,37,184,18,93,28,74,20,31,1,139,117,1,25,52,231,52,21,53,61,145,26,73,5,
    36,92,66,180,19,214,8,200,210,204,21,177,12,241,154,58,75,192,17,72,229,106,37,179,59,18,46,166,103,189,100,88,
    223,5,40,118,80,44,71,60,137,56,2,235,40,191,12,139,129,191,73,110,11,71,6,11,60,232,180,117,63,1,117,89,
    47,87,90,136,229,137,245,3,19,249,146,27,237,225,20,81,173,119,4,38,113,82,42,240,5,10,173,213,175,17,97,63,
    0,13,38,20,230,225,120,152,7,64,94,11,131,92,112,90,21,2,62,75,13,104,138,26,119,161,42,183,15,36,63,121,
    48,2,36,71,130,134,154,127,245,213,17,155,153,134,81,29,50,48,107,77,9,182,57,16,179,115,85,49,77,60,121,149,
    134,68,197,100,114,1,13,88,3,7,182,38,46,180,214,45,30,221,54,129,22,52,114,17,150,110,66,134,158,242,231,38,
    77,216,23,104,235,132,196,65,34,43,131,60,111,13,32,135,171,60,135,13,254,228,91,237,19,72,51,249,104,72,145,193,
    14,45,81,80,69,45,193,30,200,55,63,121,52,41,21,41,17,190,104,67,30,1,26,88,47,43,42,26,105,185,25,88,
    25,13,72,40,18,40,34,78,80,94,185,40,33,77,227,179,118,113,14,129,126,134,115,151,137,214,193,77,156,249,219,3,
    17,109,208,119,186,65,226,4,117,102,173,113,214,4,241,156,89,228,97,210,164,62,183,232,127,61,20,153,57,96,166,189,
    180,251,23,147,94,125,133,20,121,150,80,254,100,225,35,160,93,214,23,25,9,8,123,238,1,129,214,5,6,194,154,24,
    37,54,10,252,40,85,196,165,207,6,37,15,197,88,95,41,129,113,180,81,84,52,204,83,171,200,24,238,70,28,145,158,
    117,135,195,252,144,183,89,181,32,12,33,153,27,19,222,74,83,139,190,84,39,48,110,124,129,3,28,56,248,237,147,15,
    62,40,71,78,201,192,8,65,118,220,31,225,50,181,80,216,131,192,48,154,128,155,107,203,41,67,25,185,82,232,119,13,
    150,139,90,248,49,28,157,244,88,90,3,151,217,74,213,17,86,38,55,222,163,160,118,126,206,64,208,142,132,96,204,216,
    35,211,84,33,194,100,244,206,132,74,66,160,128,177,121,0,143,8,104,83,208,184,71,33,115,50,73,2,47,171,152,95,
    212,5,152,122,113,156,111,216,88,68,210,24,159,170,231,69,14,170,254,209,147,99,138,137,127,61,203,73,49,200,53,75,
    128,9,145,32,171,147,49,203,143,66,207,56,150,15,76,91,50,89,64,209,75,224,191,206,136,129,84,194,4,11,76,131,
    160,254,44,208,117,235,49,64,71,102,1,61,215,77,10,228,167,82,34,32,29,75,191,218,29,165,242,30,152,190,9,195,
    141,134,26,18,112,93,70,232,25,67,253,36,25,240,133,93,120,104,246,211,5,86,184,71,41,33,158,183,98,134,6,68,
    168,217,204,205,84,169,98,93,16,62,93,80,101,248,122,156,56,18,102,171,223,72,43,4,43,59,24,200,211,120,132,54,
    112,107,255,111,66,72,93,135,215,123,136,57,217,66,36,169,185,46,176,68,190,92,62,252,12,11,6,30,80,157,18,243,
    72,4,5,57,44,66,13,23,94,237,24,197,91,127,103,157,249,78,72,210,139,14,77,43,21,16,12,54,166,49,32,140
};

//------------------------------------------------------------------------------
// DenseCount

/// Point on DenseCount graph
struct DensePoint
{
    /// N: Number of blocks
    uint16_t N;

    /// Dense count found by GenerateDenseCount.cpp - Maximum value over 5 runs
    uint16_t DenseCount;
};

/// Number of points in the graph
static const unsigned kDensePointCount = 64;

/// Based on data collected with GenerateDenseCount.cpp
static const DensePoint kDensePoints[kDensePointCount] = {
    { 2048, 52 },
    { 2618, 54 },
    { 2826, 60 },
    { 3725, 62 },
    { 3962, 67 },
    { 4277, 65 },
    { 4547, 60 },
    { 5065, 64 },
    { 5224, 76 },
    { 5642, 66 },
    { 5909, 71 },
    { 6285, 76 },
    { 6583, 66 },
    { 6895, 72 },
    { 7448, 69 },
    { 7682, 76 },
    { 8046, 78 },
    { 8558, 76 },
    { 8963, 73 },
    { 9389, 81 },
    { 10143, 86 },
    { 11129, 94 },
    { 12593, 99 },
    { 12988, 105 },
    { 14032, 108 },
    { 14473, 114 },
    { 15397, 110 },
    { 16636, 113 },
    { 17698, 118 },
    { 18828, 123 },
    { 19420, 127 },
    { 20343, 136 },
    { 21979, 139 },
    { 23024, 150 },
    { 24119, 156 },
    { 25659, 162 },
    { 27298, 173 },
    { 29042, 176 },
    { 30898, 183 },
    { 31870, 190 },
    { 33906, 200 },
    { 35519, 211 },
    { 37208, 220 },
    { 38978, 234 },
    { 40205, 253 },
    { 42776, 297 },
    { 44122, 320 },
    { 45511, 336 },
    { 46944, 357 },
    { 48421, 373 },
    { 49177, 376 },
    { 50725, 380 },
    { 52321, 391 },
    { 53968, 388 },
    { 54811, 382 },
    { 54811, 382 },
    { 55667, 372 },
    { 57419, 362 },
    { 58316, 356 },
    { 60152, 347 },
    { 61091, 337 },
    { 62045, 334 },
    { 63014, 340 },
    { 64000, 345 },
};

/// Interpolate between two values of N and corresponding counts.
/// It works for Count1 < Count0
static uint16_t LinearInterpolate(
    int N0, int N1,
    int Count0, int Count1,
    int N)
{
    CAT_DEBUG_ASSERT(N >= N0 && N <= N1);

    const int numerator = (N - N0) * (Count1 - Count0);
    const int denominator = N1 - N0;

    const int count = Count0 + (unsigned)(numerator / denominator);

    CAT_DEBUG_ASSERT(count > 0);

    return static_cast<uint16_t>(count);
}

uint16_t GetDenseCount(unsigned N)
{
    DensePoint lowPoint, highPoint;

    if (N < (kTinyTableCount + kSmallTableCount)) {
        if (N < kTinyTableCount) {
            return kTinyDenseCounts[N];
        }
        else if (N <= 500) {
            lowPoint.N = 64;
            highPoint.N = 500;
            lowPoint.DenseCount = 26;
            highPoint.DenseCount = 35;
        }
        else if (N <= 1000) {
            lowPoint.N = 500;
            highPoint.N = 1000;
            lowPoint.DenseCount = 35;
            highPoint.DenseCount = 48;
        }
        else // if (N < 2048)
        {
            lowPoint.N = 1000;
            highPoint.N = 2048;
            lowPoint.DenseCount = 48;
            highPoint.DenseCount = 62;
        }
    }
    else
    {
        CAT_DEBUG_ASSERT(N >= (kTinyTableCount + kSmallTableCount) && N <= 64000);

        unsigned low = 0;
        unsigned high = kDensePointCount - 1;

        for (;;)
        {
            const unsigned mid = (high + low) / 2;
            if (mid == low) {
                break;
            }

            const DensePoint midPoint = kDensePoints[mid];

            if (N > midPoint.N) {
                low = mid;
            }
            else {
                high = mid;
            }
        }

        CAT_DEBUG_ASSERT(low < kDensePointCount);
        lowPoint = kDensePoints[low];
        CAT_DEBUG_ASSERT(low + 1 < kDensePointCount);
        highPoint = kDensePoints[low + 1];
    }

    CAT_DEBUG_ASSERT(lowPoint.N <= N);
    CAT_DEBUG_ASSERT(highPoint.N >= N);

    uint16_t dense_count = LinearInterpolate(
        lowPoint.N,
        highPoint.N,
        lowPoint.DenseCount,
        highPoint.DenseCount,
        N);

    CAT_DEBUG_ASSERT(dense_count > 0 && dense_count <= kMaxDenseCount);

    // Round up to the next D s.t. D Mod 4 = 2
    switch (dense_count % 4)
    {
    case 0: dense_count += 2; break;
    case 1: dense_count += 1; break;
    case 2: break;
    case 3: dense_count += 3; break;
    }

    return dense_count;
}


//------------------------------------------------------------------------------
// DenseSeed

const uint8_t kDenseSeeds[kDenseSeedCount] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,214,53,190,233,221,24,33,241,138,159,32,131,17,199,245,235,87,241,116,
    148,16,105,141,119,181,133,156,85,38,152,36,219,58,160,81,50,93,27,15,60,116,117,130,106,13,138,81,7,31,34,188,
    157,4,145,150,19,140,115,255,165,243,186,249,178,13,174,242,41,128,127,177,119,142,96,39,28,78,226,42,38,108,82,1,
    84,112,152,0
};

uint16_t GetDenseSeed(unsigned N, unsigned dense_count)
{
    if (N < kTinyTableCount) {
        // Get seed from tiny table (16-bit)
        return kTinyDenseSeeds[N];
    }
    else if (N < (kTinyTableCount + kSmallTableCount)) {
        // Get seed from small table (8-bit)
        return kSmallDenseSeeds[N - kTinyTableCount];
    }

    CAT_DEBUG_ASSERT(N >= kSmallTableCount && N <= 64000);
    CAT_DEBUG_ASSERT(dense_count % 4 == 2);

    const unsigned tableIndex = dense_count / 4;

    CAT_DEBUG_ASSERT(tableIndex < kDenseSeedCount);

    return kDenseSeeds[tableIndex];
}


//------------------------------------------------------------------------------
// PeelSeed

const uint8_t kPeelSeeds[kPeelSeedSubdivisions] = {
    0,13,4,7,6,13,1,6,13,7,15,13,11,1,6,7,13,5,12,11,7,14,2,10,13,3,3,3,1,9,13,7,
    2,9,10,0,2,14,14,0,13,11,5,0,15,14,7,3,15,2,1,3,7,2,15,12,9,10,14,12,5,13,1,7,
    2,4,3,15,17,11,9,10,11,4,7,15,0,14,1,2,10,7,11,3,14,1,3,14,0,1,7,2,3,2,17,10,
    3,12,0,6,4,5,9,15,11,9,18,11,8,3,6,4,5,27,9,13,5,13,5,12,4,13,2,7,16,0,4,5,
    7,4,11,12,0,8,11,9,13,5,3,4,4,7,4,7,1,0,6,14,0,6,5,11,9,1,14,7,1,19,1,6,
    12,7,0,6,17,7,15,2,11,2,1,7,13,11,14,3,4,3,2,4,22,1,17,0,13,4,8,19,9,12,1,1,
    20,15,9,14,8,2,9,5,15,3,7,3,1,4,8,12,4,2,15,9,15,22,7,7,7,9,9,0,5,4,1,6,
    12,13,9,7,12,14,3,8,0,9,16,3,7,3,0,15,7,7,6,7,3,3,12,0,12,13,5,6,0,10,14,9,
    7,1,15,2,2,11,12,11,3,6,18,11,7,13,12,0,7,4,11,2,9,14,15,11,10,9,1,7,0,10,7,6,
    5,18,0,9,4,1,6,3,0,7,21,1,2,7,3,15,5,0,3,10,9,0,18,10,1,1,17,0,4,18,2,12,
    4,2,2,2,9,8,9,2,9,3,5,11,9,3,9,3,15,2,1,5,9,15,9,4,10,12,2,2,6,5,11,2,
    9,5,6,6,14,10,10,22,9,4,6,10,0,12,6,18,11,3,4,15,3,5,10,3,7,0,24,20,10,2,7,3,
    5,8,6,0,10,2,16,10,5,9,9,3,0,13,2,14,2,0,4,0,3,4,4,12,12,2,3,0,1,3,7,2,
    2,5,14,7,5,0,8,9,13,4,13,5,4,9,19,2,13,0,11,1,3,4,0,3,3,4,3,4,15,0,6,5,
    9,15,0,22,3,2,7,5,10,9,2,3,17,3,9,6,2,2,4,15,11,10,2,1,6,3,8,13,9,9,0,2,
    7,1,6,13,7,0,9,1,16,2,13,9,0,12,4,11,2,3,0,0,2,9,11,12,11,18,1,0,4,21,3,7,
    2,5,7,5,4,1,12,0,14,3,17,8,11,8,9,5,8,0,3,3,7,11,8,3,15,1,15,3,7,3,17,10,
    7,18,11,0,2,9,10,4,16,17,0,7,7,4,8,13,7,12,7,0,1,19,15,3,4,12,18,8,2,3,0,13,
    22,6,14,3,1,7,8,2,6,6,10,18,12,12,7,15,1,0,9,5,11,4,2,0,1,0,6,11,2,8,13,17,
    4,1,8,12,12,1,3,1,5,19,5,12,1,11,11,3,8,3,12,15,6,13,5,0,5,10,1,8,5,7,11,3,
    22,5,9,12,1,17,0,7,12,1,8,8,3,17,3,10,5,5,3,4,3,0,9,4,1,0,9,7,4,9,1,7,
    1,4,3,3,7,11,4,9,9,8,8,5,1,12,10,7,7,0,0,10,4,12,4,4,2,6,13,7,7,13,8,2,
    0,7,3,12,7,4,7,4,8,7,8,7,3,1,12,19,17,4,1,4,12,1,0,15,0,8,0,9,1,18,0,0,
    7,3,2,10,12,1,11,11,18,1,17,3,13,4,6,0,9,0,1,5,2,2,10,3,10,12,1,3,5,2,2,5,
    1,5,3,1,25,0,18,0,4,2,9,10,10,5,7,10,8,7,9,1,4,0,0,1,14,10,7,15,4,3,7,1,
    12,0,6,15,1,4,12,9,3,17,1,2,16,7,0,10,1,9,10,10,11,0,7,4,17,11,4,5,16,10,0,10,
    1,3,9,6,1,5,15,5,9,0,3,14,6,1,9,1,6,9,5,9,0,3,9,2,9,4,7,1,1,5,0,12,
    6,8,11,0,1,1,5,1,7,7,11,2,11,1,7,2,4,1,7,4,15,12,1,17,16,12,3,1,10,11,12,7,
    11,11,4,18,6,5,3,10,7,3,4,2,6,9,15,9,17,4,8,3,4,4,4,4,0,1,12,1,13,13,4,7,
    7,9,9,19,1,12,16,4,14,1,5,8,9,12,2,12,15,11,10,5,1,13,6,7,8,9,1,0,9,6,14,1,
    15,1,5,13,7,15,9,3,1,14,2,10,9,11,7,2,18,11,0,8,16,20,7,11,14,22,13,1,11,15,7,5,
    7,8,6,7,6,1,11,6,12,2,6,6,1,10,2,9,15,7,8,0,12,10,17,1,0,7,12,0,2,2,18,17,
    0,12,0,3,1,7,12,11,6,5,0,5,4,3,3,11,22,1,3,0,7,14,11,3,0,2,6,19,6,10,11,3,
    8,4,7,5,1,2,4,11,10,4,9,9,3,8,1,3,15,17,1,0,5,15,0,7,2,10,1,4,3,1,8,9,
    12,2,6,7,9,7,1,4,0,10,11,2,12,4,11,3,1,13,3,2,11,4,1,5,6,7,7,12,7,4,5,10,
    9,11,0,9,0,11,2,6,4,3,0,10,1,17,9,10,15,6,5,11,4,3,11,0,13,9,4,1,14,6,14,7,
    4,3,1,1,0,4,5,11,2,5,2,4,13,7,19,10,3,10,14,4,5,3,3,4,17,2,5,20,1,9,1,18,
    7,7,3,8,9,12,5,9,4,11,1,12,17,0,12,11,21,13,9,18,4,4,7,13,1,17,0,12,3,7,0,9,
    16,9,13,11,3,10,5,0,13,9,0,6,6,1,12,3,1,3,9,10,2,15,8,4,11,7,0,9,15,12,0,12,
    4,4,7,2,0,4,3,1,0,15,9,0,0,12,3,6,0,10,17,10,5,9,14,2,2,7,8,7,10,9,7,7,
    4,1,12,12,5,12,9,6,7,12,3,9,14,4,2,4,6,4,8,3,1,1,5,7,0,11,7,14,7,2,24,11,
    4,13,9,5,0,17,5,2,14,7,1,7,8,1,9,2,1,1,7,12,10,21,1,17,4,9,7,12,0,1,9,0,
    2,6,1,4,3,0,11,6,9,11,12,0,2,8,0,0,8,9,1,0,19,13,3,0,3,0,7,6,12,2,6,9,
    12,4,1,4,12,10,15,4,3,9,15,1,14,4,12,9,3,4,15,0,15,7,1,9,0,9,3,4,7,17,1,6,
    8,4,17,7,0,0,11,2,12,17,3,13,7,3,16,13,0,2,8,8,8,9,7,13,14,19,4,7,12,11,11,4,
    6,7,11,0,0,12,2,11,14,15,3,5,9,0,13,3,0,7,3,8,7,14,14,15,3,0,13,8,11,1,7,9,
    3,9,11,18,7,3,12,4,9,21,0,1,8,0,6,7,12,3,1,2,12,13,3,2,9,1,9,8,10,7,11,15,
    17,7,0,5,11,4,13,8,12,1,3,1,0,11,5,5,5,8,7,5,3,8,9,5,9,8,1,7,15,4,2,1,
    14,7,10,17,16,1,18,8,8,0,2,1,15,0,1,3,4,3,7,4,14,15,10,9,1,9,18,3,0,1,5,11,
    11,10,7,7,9,4,9,9,1,18,10,3,20,0,0,9,0,3,7,7,4,21,9,15,7,5,10,7,0,6,17,7,
    5,16,7,11,10,10,0,6,7,7,4,0,12,4,1,2,9,2,8,3,7,2,3,0,15,11,7,7,4,17,9,3,
    1,4,4,1,9,7,5,1,7,4,6,18,4,4,0,14,3,7,11,13,13,4,11,2,5,6,9,6,11,8,6,0,
    11,7,5,0,3,8,1,0,8,1,4,7,11,0,8,8,15,9,1,19,4,10,3,11,3,19,1,2,1,2,1,9,
    7,10,1,0,3,24,4,11,15,9,11,0,3,11,9,3,13,4,12,11,1,7,15,2,1,0,3,1,6,7,11,2,
    4,1,9,1,5,1,7,12,7,5,9,1,7,9,6,0,6,9,0,1,14,21,15,11,5,9,5,10,7,12,16,10,
    10,3,16,7,8,12,1,2,0,8,1,2,1,5,0,4,4,0,4,0,14,3,2,17,6,10,17,4,19,6,0,0,
    9,5,3,0,4,10,1,1,5,2,9,1,7,4,0,4,2,4,2,11,2,5,4,5,6,1,7,4,0,11,11,3,
    13,8,12,1,0,1,14,7,9,11,1,7,4,7,7,0,13,9,19,12,8,5,8,1,5,10,1,7,2,12,6,0,
    15,13,2,0,8,7,13,7,0,7,1,5,1,5,4,13,3,8,0,0,0,1,6,14,4,0,1,12,4,5,9,15,
    15,9,5,2,19,1,0,3,4,2,0,4,4,3,20,9,3,4,12,0,6,4,4,4,3,1,12,7,0,5,3,10,
    6,9,8,0,3,14,9,3,0,1,11,3,12,7,0,7,7,4,8,8,10,7,14,10,0,0,15,5,2,8,9,6,
    4,22,16,13,11,3,6,6,14,1,3,4,7,17,14,5,9,3,9,0,1,12,6,2,7,0,0,5,1,7,7,4,
    4,4,21,3,3,4,12,0,9,14,4,12,0,9,4,14,14,1,7,3,14,19,7,2,11,3,1,3,14,11,4,1,
    9,0,1,4,1,12,5,0,2,14,13,0,1,1,4,0,14,11,16,6,3,7,6,13,13,0,1,8,0,2,1,8,
};

uint16_t GetPeelSeed(unsigned N)
{
    if (N < (kTinyTableCount + kSmallTableCount)) {
        return kSmallPeelSeeds[N];
    }

    CAT_DEBUG_ASSERT(N >= kSmallTableCount && N <= 64000);

    const unsigned subdivision = N % kPeelSeedSubdivisions;

    // Get seed from subdivided table
    return kPeelSeeds[subdivision];
}


} // namespace wirehair

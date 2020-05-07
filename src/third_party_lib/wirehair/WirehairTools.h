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

#ifndef WIREHAIR_TOOLS_H
#define WIREHAIR_TOOLS_H

#include "wirehair.h"
#include "gf256.h"
#include <new> // std::nothrow

// Compiler-specific debug break
#if defined(_DEBUG) || defined(DEBUG)
    #define CAT_DEBUG
    #ifdef _WIN32
        #define CAT_DEBUG_BREAK() __debugbreak()
    #else
        #define CAT_DEBUG_BREAK() __builtin_trap()
    #endif
    #define CAT_DEBUG_ASSERT(cond) { if (!(cond)) { CAT_DEBUG_BREAK(); } }
#else
    #define CAT_DEBUG_BREAK() do {} while (false);
    #define CAT_DEBUG_ASSERT(cond) do {} while (false);
#endif

namespace wirehair {


//------------------------------------------------------------------------------
// Configuration

// Debugging:
//#define CAT_DUMP_CODEC_DEBUG    /**< Turn on debug output for decoder */
//#define CAT_DUMP_ROWOP_COUNTERS /**< Dump row operations counters to console */
//#define CAT_DUMP_PIVOT_FAIL     /**< Dump pivot failure to console */
//#define CAT_DUMP_GE_MATRIX      /**< Dump GE matrix to console */

// Limits:
#define CAT_REF_LIST_MAX   32    /**< Tune to be as small as possible and still succeed */
#define CAT_MAX_DENSE_ROWS 500   /**< Maximum dense row count */
#define CAT_MAX_EXTRA_ROWS 32    /**< Maximum number of extra rows to support before reusing existing rows */
#define CAT_WIREHAIR_MAX_N 64000 /**< Largest N value to allow */
#define CAT_WIREHAIR_MIN_N 2     /**< Smallest N value to allow */

// Optimization options:
#define CAT_COPY_FIRST_N      /**< Copy the first N rows from the input (faster) */
#define CAT_HEAVY_WIN_MULT    /**< Use 4-bit table and multiplication optimization (faster) */
#define CAT_WINDOWED_BACKSUB  /**< Use window optimization for back-substitution (faster) */
#define CAT_WINDOWED_LOWERTRI /**< Use window optimization for lower triangle elimination (faster) */
#define CAT_ALL_ORIGINAL      /**< Avoid doing calculations for 0 losses -- Requires CAT_COPY_FIRST_N (faster) */

/// Number of heavy rows at the bottom of the matrix
static const unsigned kHeavyRows = 6;

/// Number of heavy columns at the bottom right of the matrix
static const unsigned kHeavyCols = 18;

// If the heavy row/column count changes then we will have to regenerate a new
// heavy matrix using HeavyRowGenerator.cpp, and all the dense/peel seeds.


//------------------------------------------------------------------------------
// PCG PRNG

/// From http://www.pcg-random.org/
class PCGRandom
{
public:
    void Seed(uint64_t y, uint64_t x = 0)
    {
        State = 0;
        Inc = (y << 1u) | 1u;
        Next();
        State += x;
        Next();
    }

    GF256_FORCE_INLINE uint32_t Next()
    {
        const uint64_t oldstate = State;
        State = oldstate * UINT64_C(6364136223846793005) + Inc;
        const uint32_t xorshifted = (uint32_t)(((oldstate >> 18) ^ oldstate) >> 27);
        const uint32_t rot = oldstate >> 59;
        return (xorshifted >> rot) | (xorshifted << ((uint32_t)(-(int32_t)rot) & 31));
    }

    uint64_t State = 0, Inc = 0;
};


//------------------------------------------------------------------------------
// Bit-Rotation Macros

/// These bit rotations only work for uint64_t
#define CAT_ROL64(n, r) ( ((uint64_t)(n) << (r)) | ((uint64_t)(n) >> (64 - (r))) )
#define CAT_ROR64(n, r) ( ((uint64_t)(n) >> (r)) | ((uint64_t)(n) << (64 - (r))) )


//------------------------------------------------------------------------------
// Utility: 16-bit Integer Square Root function

/**
    FloorSquareRoot16()

    16-bit Integer Square Root function = (uint16_t)(sqrt(x))

    This is useful for prime testing, where the algorithm needs to stop testing
    at or before the square root of the number to test.
*/
uint16_t FloorSquareRoot16(uint16_t x);


//------------------------------------------------------------------------------
// Utility: 16-bit Truncated Sieve of Eratosthenes Next Prime function

/**
    NextPrime16()

    16-bit Truncated Sieve of Eratosthenes Next Prime function

    It uses trial division up to the square root of the number to test.
    Uses a truncated sieve table to pick the next number to try, which
    avoids small factors 2, 3, 5, 7.  This can be considered a more
    involved version of incrementing by 2 instead of 1.  It takes about
    25% less time on average than the approach that just increments by 2.

    Because of the tabular increment this is a hybrid approach.  The sieve
    would just use a very large table, but I wanted to limit the size of
    the table to something fairly small and reasonable.  210 bytes for the
    sieve table.  102 bytes for the primes list.

    It also calculates the integer square root faster than cmath sqrt()
    and uses multiplication to update the square root instead for speed.

    The largest prime number within 16 bits is 65521.

    If input is 0 or 1, it returns 1.
    If input is 2, it returns 2.
    If input is a prime, it will return the same number.
    If input is above 65521, it will return 0 and assert in debug mode.
    Otherwise it will return the next larger number that is prime.
*/
uint16_t NextPrime16(uint16_t n);


//------------------------------------------------------------------------------
// Utility: GF(2) Invertible Matrix Generator function

/**
    AddInvertibleGF2Matrix()

    This function adds (xoring in) a binary matrix on top of the given one.

    This function generates random-looking invertible NxN binary matrices for
    N in the range [1, 512).  For larger values of N it will just add the
    identity matrix.

    The input matrix is expected to be stored row-first in memory.
    The offset to the first column bit to add can be specified.

    Precondition: N != 0
*/
void AddInvertibleGF2Matrix(
    uint64_t * GF256_RESTRICT matrix, ///< Address of upper left word
    const unsigned offset, ///< Offset in bits to the first sum column
    const unsigned pitchWords, ///< Pitch of the matrix in words
    const unsigned n ///< Number of bits in the matrix to generate
);


//------------------------------------------------------------------------------
// Utility: Deck Shuffling function

/**
    ShuffleDeck16()

    Given a PRNG, generate a deck of cards in a random order.

    The deck will contain elements with values between 0 and count - 1.
*/
void ShuffleDeck16(
    PCGRandom &prng,
    uint16_t * GF256_RESTRICT deck,
    const uint32_t count);


//------------------------------------------------------------------------------
// Utility: Column Iterator function

/**
    IterateNextColumn()

    This implements a very light PRNG (Weyl function) to quickly generate
    a set of random-looking columns without replacement.

    This is Stewart Platt's excellent loop-less iterator optimization.
    His common cases all require no additional modulus operation, which
    makes it faster than the rare case that I designed.
*/
GF256_FORCE_INLINE void IterateNextColumn(
    uint16_t &x,      ///< Current column number to mutate
    const uint16_t b, ///< Non-prime modulus for addition
    const uint16_t p, ///< Next prime above b (or = b if prime)
    const uint16_t a  ///< Number to add for Weyl generator
)
{
    x = (x + a) % p;

    // If the result is in the range [b, p):
    if (x >= b)
    {
        const uint16_t distanceToP = p - x;

        // If adding again would wrap around:
        if (a >= distanceToP) {
            x = a - distanceToP;
        }
        else {
            // Handle rare case where multiple adds are needed
            x = (((uint32_t)a << 16) - distanceToP) % a;
        }
    }
}


/**
    GeneratePeelRowWeight()

    Ideal Soliton weight distribution from
    "LT Codes" (2002) by Michael Luby

    The weight distribution selected for use in this codec is the
    Ideal Soliton distribution.  The PMF for weights 2 and higher
    is 1 / (k*(k - 1)).  Accumulating these yields the WEIGHT_DIST
    table below up to weight 64.  I stuck ~0 at the end of the
    table to make sure the while loop in the function terminates.

    To produce a good code, the probability of weight-1 should be
    added into each element of the WEIGHT_DIST table.  Instead,
    I add it programmatically in the function to allow it to be
    easily tuned.

    I played around with where to truncate this table, and found
    that for higher block counts, the number of deferred rows after
    greedy peeling is much lower for 64 weights than 32.  And after
    tuning the codec for weight 64, the performance was slightly
    better than with 32.

    I also tried different probabilities for weight-1 rows, and
    settled on 1/128 as having the best performance in a few select
    tests.  Setting it too high or too low (or even to zero) tends
    to reduce the performance of the codec.

    However, once N gets much much larger, it is actually very
    beneficial to switch over to weight-2 as a minimum.
*/
uint16_t GeneratePeelRowWeight(
    uint32_t rv, ///< 32-bit random value
    uint16_t block_count ///< Number of input blocks
);


//------------------------------------------------------------------------------
// Utility: Peel Matrix Row Parameter Initialization

struct PeelRowParameters
{
    /// Peeling matrix: Column generator
    uint16_t PeelCount, PeelFirst, PeelAdd;

    /// Mixing matrix: Column generator
    uint16_t MixFirst, MixAdd;

    /// This function generates the PRNG parameters for randomly selecting which
    /// columns to add into the given row
    void Initialize(
        uint32_t row_seed,
        uint32_t p_seed,
        uint16_t peel_column_count,
        uint16_t mix_column_count);
};


//------------------------------------------------------------------------------
// Utility: Peel Row Iterator

/// Generates all of the columns referenced by a row
class PeelRowIterator
{
public:
    /// Initialize iterator with parameters
    GF256_FORCE_INLINE PeelRowIterator(
        const PeelRowParameters& params,
        uint16_t columnCount,
        uint16_t columnCountNextPrime)
    {
        CAT_DEBUG_ASSERT(columnCountNextPrime >= columnCount);
        CAT_DEBUG_ASSERT(params.PeelFirst < columnCount);
        CAT_DEBUG_ASSERT(params.PeelCount > 0);
        CAT_DEBUG_ASSERT(params.PeelAdd > 0);
        CAT_DEBUG_ASSERT(columnCount > 0);
        CAT_DEBUG_ASSERT(params.PeelCount <= CAT_MAX_DENSE_ROWS)

        NextColumn = params.PeelFirst;
        ColumnsRemaining = params.PeelCount - 1;
        Adder = params.PeelAdd;
        ColumnCount = columnCount;
        ColumnCountNextPrime = columnCountNextPrime;
    }

    /// Get current column
    GF256_FORCE_INLINE uint16_t GetColumn() const
    {
        return NextColumn;
    }

    /// Get the next column.
    /// Returns true on success.
    /// Returns false if no more columns remain
    GF256_FORCE_INLINE bool Iterate()
    {
        if (ColumnsRemaining <= 0) {
            return false;
        }

        --ColumnsRemaining;

        IterateNextColumn(
            NextColumn,
            ColumnCount,
            ColumnCountNextPrime,
            Adder);
        return true;
    }

protected:
    /// Number of column references remaining in this row
    uint16_t ColumnsRemaining = 0;

    /// Next column to output
    uint16_t NextColumn = 0;

    /// Weyl generator adder
    uint16_t Adder = 0;

    /// Number of columns overall
    uint16_t ColumnCount = 0;

    /// Next prime starting at number of columns
    uint16_t ColumnCountNextPrime = 0;
};


//------------------------------------------------------------------------------
// Utility: Row Mix Iterator

/// Generates all of the columns referenced by a row
class RowMixIterator
{
public:
    /// Initialize iterator with parameters
    GF256_FORCE_INLINE RowMixIterator(
        const PeelRowParameters& params,
        uint16_t columnCount,
        uint16_t columnCountNextPrime)
    {
        CAT_DEBUG_ASSERT(columnCountNextPrime >= columnCount);
        CAT_DEBUG_ASSERT(params.MixFirst < columnCount);
        CAT_DEBUG_ASSERT(params.MixAdd > 0);
        CAT_DEBUG_ASSERT(columnCount > 0);

        uint16_t x = params.MixFirst;
        Columns[0] = x;

        for (unsigned i = 1; i < kColumnCount; ++i)
        {
            IterateNextColumn(
                x,
                columnCount,
                columnCountNextPrime,
                params.MixAdd);

            Columns[i] = x;
        }
    }

    /// Number of output columns
    static const unsigned kColumnCount = 3;

    /// Output columns
    uint16_t Columns[kColumnCount];
};


//------------------------------------------------------------------------------
// SIMD-Safe Aligned Memory Allocations

/// Allocate memory and return a pointer aligned to the size used for SIMD ops
uint8_t* SIMDSafeAllocate(size_t size);

/// Free an aligned pointer
void SIMDSafeFree(void* ptr);


//------------------------------------------------------------------------------
// Tables for small N

/// These tables were lovingly hand-crafted by hard-working indigenous peoples:

static const unsigned kTinyTableCount = 65; // 0..64

static const unsigned kSmallTableCount = 2048 - kTinyTableCount;

/// This table maps N -> dense row count
extern const uint8_t kTinyDenseCounts[kTinyTableCount];

/// This table maps N -> dense seed
extern const uint16_t kTinyDenseSeeds[kTinyTableCount];

/// This table maps (N - kTinyTableCount) -> dense seed
extern const uint8_t kSmallDenseSeeds[kSmallTableCount];

/// This table maps N -> peel seed
extern const uint8_t kSmallPeelSeeds[kTinyTableCount + kSmallTableCount];


//------------------------------------------------------------------------------
// Tables for larger N

static const unsigned kDenseSeedCount = 100;

/// This table maps from (N / 4) -> dense seed for N >= 2048
extern const uint8_t kDenseSeeds[kDenseSeedCount];

static const unsigned kPeelSeedSubdivisions = 2048;

/// This table maps from N % kPeelSeedSubdivisions -> peel seed
extern const uint8_t kPeelSeeds[kPeelSeedSubdivisions];


//------------------------------------------------------------------------------
// DenseCount

static const unsigned kMaxDenseCount = 400;

/**
    This function returns the number of dense rows in the matrix given the
    number of input blocks (N).  The dense rows are supposed to make a random
    binary submatrix where it is equally likely to have a 1 or a 0.  If the
    peel rows fail to contain a data piece it's much more likely to find that
    piece in these rows.  Whenever the peel row parameters are re-tuned, the
    optimal dense count for each N must be re-computed.

    For N < 2048, a thorough search is performed by GenerateSmallDenseSeeds.cpp
    to find the best match of dense seed and dense count.

    For larger values of N up to 64000, the dense count is generated by the
    GenerateDenseCount.cpp program.  This program was run 5 times and the
    output graphed.  The largest dense count from each run was used and
    approximated by 64 data points that overestimate the dense count needed
    for the other values of N.

    The binary search used is unit-tested in TableGenerator.cpp with array
    bounds checks.

    For N >= 2048 the graph-based value is rounded up to the next value s.t.
    D Mod 4 = 2.  This is because I found that on average these counts lead
    to much better results.

    Preconditions: N >= 2 and N <= 64000
*/
uint16_t GetDenseCount(unsigned N);


//------------------------------------------------------------------------------
// DenseSeed

/**
    This function returns the seed to use for the dense rows, given the number
    of input blocks (N) and the count of dense rows provided by GetDenseCount().
*/
uint16_t GetDenseSeed(unsigned N, unsigned dense_count);


//------------------------------------------------------------------------------
// PeelSeed

/**
    This function returns the seed to use for the peel rows.
*/
uint16_t GetPeelSeed(unsigned N);


} // namespace wirehair

#endif // WIREHAIR_TOOLS_H

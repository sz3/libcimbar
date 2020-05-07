/** \file
    \brief Wirehair : Table Generator
    \copyright Copyright (c) 2018 Christopher A. Taylor.  All rights reserved.

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

#include "HeavyRowGenerator.h"

#include "../test/SiameseTools.h"
using namespace siamese;

#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cmath>
using namespace std;

#ifdef _MSC_VER
#include <intrin.h> // _BitScanReverse
#pragma intrinsic(_BitScanReverse)
#endif

// Compiler-specific debug break
#if defined(_DEBUG) || defined(DEBUG)
    #define TABLEGEN_DEBUG
    #ifdef _WIN32
        #define TABLEGEN_DEBUG_BREAK() __debugbreak()
    #else
        #define TABLEGEN_DEBUG_BREAK() __builtin_trap()
    #endif
    #define TABLEGEN_DEBUG_ASSERT(cond) { if (!(cond)) { TABLEGEN_DEBUG_BREAK(); } }
#else
    #define TABLEGEN_DEBUG_BREAK() do {} while (false);
    #define TABLEGEN_DEBUG_ASSERT(cond) do {} while (false);
#endif


//------------------------------------------------------------------------------
// Stats

// Return percentile stats, 0.5 = median by default
// This re-orders the input vector.
static uint32_t CalculatePercentile(
    std::vector<uint32_t>& samples,
    float percentile = 0.5f)
{
    using offset_t = std::vector<uint32_t>::size_type;
    const unsigned kSampleCount = (unsigned)samples.size();

    offset_t goalOffset = (offset_t)(percentile * kSampleCount);
    std::nth_element(
        samples.begin(),
        samples.begin() + goalOffset,
        samples.begin() + kSampleCount,
        [](uint32_t a, uint32_t b)->bool
    {
        return a < b;
    });

    return samples[goalOffset];
}


/*
    This is a unified implementation of my favorite generator
    that is designed to generate up to 2^^32 numbers per seed.

    Its period is about 2^^126 and passes all BigCrush tests.
    It is the fastest generator I could find that passes all tests.

    Furthermore, the input seeds are hashed to avoid linear
    relationships between the input seeds and the low bits of
    the first few outputs.
*/
class Abyssinian
{
    uint64_t _x, _y;

public:
    inline void Initialize(uint32_t x, uint32_t y)
    {
        // Based on the mixing functions of MurmurHash3
        static const uint64_t C1 = 0xff51afd7ed558ccdULL;
        static const uint64_t C2 = 0xc4ceb9fe1a85ec53ULL;

        x += y;
        y += x;

        uint64_t seed_x = 0x9368e53c2f6af274ULL ^ x;
        uint64_t seed_y = 0x586dcd208f7cd3fdULL ^ y;

        seed_x *= C1;
        seed_x ^= seed_x >> 33;
        seed_x *= C2;
        seed_x ^= seed_x >> 33;

        seed_y *= C1;
        seed_y ^= seed_y >> 33;
        seed_y *= C2;
        seed_y ^= seed_y >> 33;

        _x = seed_x;
        _y = seed_y;

        // Inlined Next(): Discard first output

        _x = (uint64_t)0xfffd21a7 * (uint32_t)_x + (uint32_t)(_x >> 32);
        _y = (uint64_t)0xfffd1361 * (uint32_t)_y + (uint32_t)(_y >> 32);
    }

    inline void Initialize(uint32_t seed)
    {
        Initialize(seed, seed);
    }

    inline uint32_t Next()
    {
        _x = (uint64_t)0xfffd21a7 * (uint32_t)_x + (uint32_t)(_x >> 32);
        _y = (uint64_t)0xfffd1361 * (uint32_t)_y + (uint32_t)(_y >> 32);
#define CAT_ROL32(n, r) ( ((uint32_t)(n) << (r)) | ((uint32_t)(n) >> (32 - (r))) )
        return CAT_ROL32((uint32_t)_x, 7) + (uint32_t)_y;
    }
};

void Benchmark_PRNG()
{
    // Collect many samples to account for noise
    static const unsigned kSampleCount = 500;
    std::vector<uint32_t> pcgDelay;
    std::vector<uint32_t> abyssinianDelay;
    pcgDelay.reserve(kSampleCount);
    abyssinianDelay.reserve(kSampleCount);

    // Need to run each algorithm many times per bin
    static const unsigned kRunsPerSample = 1000;

    static const unsigned kOutputs = 16;

    unsigned sum = 0;

    for (unsigned sample = 1; sample <= kSampleCount; ++sample)
    {
        const uint64_t t0 = GetTimeUsec();

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            PCGRandom prng;
            prng.Seed(sample, run);
            for (unsigned outputs = 0; outputs < kOutputs; ++outputs) {
                sum ^= prng.Next();
            }
        }

        const uint64_t t1 = GetTimeUsec();

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            Abyssinian prng;
            prng.Initialize(sample, run);
            for (unsigned outputs = 0; outputs < kOutputs; ++outputs) {
                sum ^= prng.Next();
            }
        }

        const uint64_t t2 = GetTimeUsec();

        if (sample > 10)
        {
            pcgDelay.push_back((uint32_t)(t1 - t0));
            abyssinianDelay.push_back((uint32_t)(t2 - t1));
        }
    }

    if (sum != 0xe887ecc4)
    {
        cout << "PRNG test failed!" << endl;
        TABLEGEN_DEBUG_BREAK();
    }

    cout << "PRNG timing results:" << endl;

    const uint32_t pcg25 = CalculatePercentile(pcgDelay, 0.25f);
    const uint32_t pcg50 = CalculatePercentile(pcgDelay, 0.5f);
    const uint32_t pcg75 = CalculatePercentile(pcgDelay, 0.75f);

    cout << "* PCG algorithm: 25% latency = " << pcg25 << " usec" << endl;
    cout << "* PCG algorithm: 50% latency = " << pcg50 << " usec (median)" << endl;
    cout << "* PCG algorithm: 75% latency = " << pcg75 << " usec" << endl;

    const uint32_t abyssinian25 = CalculatePercentile(abyssinianDelay, 0.25f);
    const uint32_t abyssinian50 = CalculatePercentile(abyssinianDelay, 0.5f);
    const uint32_t abyssinian75 = CalculatePercentile(abyssinianDelay, 0.75f);

    cout << "* Abyssinian algorithm: 25% latency = " << abyssinian25 << " usec" << endl;
    cout << "* Abyssinian algorithm: 50% latency = " << abyssinian50 << " usec (median)" << endl;
    cout << "* Abyssinian algorithm: 75% latency = " << abyssinian75 << " usec" << endl;
    cout << endl;
}


//------------------------------------------------------------------------------
// Utility: 16-bit Integer Square Root function

static uint8_t kSquareRootTable[256];

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

// Algorithm from WirehairTools.cpp
static uint16_t FloorSquareRoot16(uint16_t x)
{
#if 1

    unsigned nonzeroBits;
    if (x < 0x100) {
        nonzeroBits = 6;
    }
    else {
        nonzeroBits = Log32(x);
    }

    // x < 0x100 : tab 0 shift 4
    //    0 <= nonzeroBits <= 8
    // x < 0x400 : tab 2 shift 3
    //    9 <= nonzeroBits <= 10
    // x < 0x1000 : tab 4 shift 2
    //    11 <= nonzeroBits <= 12
    // x < 0x4000 : tab 6 shift 1
    //    13 <= nonzeroBits <= 14
    // x >=0x4000 : tab 8 shift 0
    //    15 <= nonzeroBits <= 16

    // Choose the shifts based on number of bits in input
    const unsigned tableShift = (nonzeroBits - 6) & 0xfe;
    const unsigned resultShift = (15 - nonzeroBits) / 2;

    uint16_t r = kSquareRootTable[x >> tableShift] >> resultShift;

#else

    if (x < 0x100) {
        return kSquareRootTable[x] >> 4;
    }

    uint16_t r;

    if (x >= 0x1000)
    {
        if (x >= 0x4000) {
            r = kSquareRootTable[x >> 8];
        }
        else {
            r = (kSquareRootTable[x >> 6] >> 1);
        }
    }
    else
    {
        if (x >= 0x400) {
            r = (kSquareRootTable[x >> 4] >> 2);
        }
        else {
            r = (kSquareRootTable[x >> 2] >> 3);
        }
    }

#endif

    // Correct rounding if necessary (compiler optimizes this form better)
    r -= (r * r > x);

    return r;
}

static void Generate_kSquareRootTable()
{
    unsigned root = 0;
    unsigned next_root_squared = 1 * 1;

    // Find square root for each value:
    for (unsigned x = 0; x < 65536; ++x)
    {
        while (next_root_squared <= x)
        {
            ++root;

            const unsigned next_root = root + 1;
            next_root_squared = next_root * next_root;
        }

        if (x >= 0x4000) {
            kSquareRootTable[x >> 8] = (uint8_t)root;
        }
        else if (x >= 0x1000) {
            kSquareRootTable[x >> 6] = (uint8_t)(root << 1);
        }
        else if (x >= 0x400) {
            kSquareRootTable[x >> 4] = (uint8_t)(root << 2);
        }
        else if (x >= 0x100) {
            kSquareRootTable[x >> 2] = (uint8_t)(root << 3);
        }
        else {
            kSquareRootTable[x] = (uint8_t)(root << 4);
        }
    }
}

static bool Validate_FloorSquareRoot16()
{
    for (unsigned i = 0; i < 65536; ++i)
    {
        const unsigned x = FloorSquareRoot16((uint16_t)i);
        const unsigned expected = (uint16_t)sqrt(i);

        if (x != expected) {
            TABLEGEN_DEBUG_BREAK(); // Failed
            return false;
        }
    }

    return true;
}

static void Benchmark_FloorSquareRoot16()
{
    // Collect many samples to account for noise
    static const unsigned kSampleCount = 500;
    std::vector<uint32_t> referenceDelay;
    std::vector<uint32_t> customDelay;
    referenceDelay.reserve(kSampleCount);
    customDelay.reserve(kSampleCount);

    // Need to run each algorithm many times per bin
    static const unsigned kRunsPerSample = 10000;

    std::vector<uint16_t> sampleInputs;
    sampleInputs.resize(kRunsPerSample);

    for (unsigned sample = 1; sample <= kSampleCount; ++sample)
    {
        unsigned sum = 0;

        PCGRandom prng;

        prng.Seed(sample);

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = (uint16_t)prng.Next();
            sampleInputs[run] = value;
        }

        const uint64_t t0 = GetTimeUsec();

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = sampleInputs[run];
            const unsigned x = FloorSquareRoot16(value);
            sum ^= x;
        }

        const uint64_t t1 = GetTimeUsec();

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = sampleInputs[run];
            const unsigned expected = (uint16_t)sqrt(value);
            sum ^= expected;
        }

        const uint64_t t2 = GetTimeUsec();

        if (sum != 0)
        {
            cout << "Test failed!" << endl;
            TABLEGEN_DEBUG_BREAK();
        }

        if (sample > 10)
        {
            customDelay.push_back((uint32_t)(t1 - t0));
            referenceDelay.push_back((uint32_t)(t2 - t1));
        }
    }

    cout << "FloorSquareRoot16 timing results vs reference algorithm:" << endl;

    const uint32_t custom25 = CalculatePercentile(customDelay, 0.25f);
    const uint32_t custom50 = CalculatePercentile(customDelay, 0.5f);
    const uint32_t custom75 = CalculatePercentile(customDelay, 0.75f);

    cout << "* Custom algorithm: 25% latency = " << custom25 << " usec" << endl;
    cout << "* Custom algorithm: 50% latency = " << custom50 << " usec (median)" << endl;
    cout << "* Custom algorithm: 75% latency = " << custom75 << " usec" << endl;

    const uint32_t reference25 = CalculatePercentile(referenceDelay, 0.25f);
    const uint32_t reference50 = CalculatePercentile(referenceDelay, 0.5f);
    const uint32_t reference75 = CalculatePercentile(referenceDelay, 0.75f);

    cout << "* Reference algorithm: 25% latency = " << reference25 << " usec" << endl;
    cout << "* Reference algorithm: 50% latency = " << reference50 << " usec (median)" << endl;
    cout << "* Reference algorithm: 75% latency = " << reference75 << " usec" << endl;
    cout << endl;
}

static void Print_kSquareRootTable()
{
    cout << endl;
    cout << "static const uint8_t kSquareRootTable[256] = {" << endl;

    for (unsigned i = 0; i < 256 / 16; ++i)
    {
        cout << "    ";

        for (unsigned j = 0; j < 15; ++j) {
            cout << (int)kSquareRootTable[i * 16 + j] << ", ";
        }

        cout << (int)kSquareRootTable[i * 16 + 15];
        if (i < 256 / 16) {
            cout << ",";
        }
        cout << endl;
    }

    cout << "};" << endl;
    cout << endl;
}


//------------------------------------------------------------------------------
// Utility: 16-bit Truncated Sieve of Eratosthenes Next Prime function

// Reference function to check if a number is prime.
// The point of reference code is to be simple and hard to mess up,
// rather than to be clever or fast.
static bool IsPrime(unsigned n)
{
    if (0 == (n % 2)) {
        return false;
    }

#if 1

    // This is significantly faster
    unsigned p_max = FloorSquareRoot16((uint16_t)n);

    for (unsigned i = 3; i <= p_max; i += 2) {
        if (0 == (n % i)) {
            return false;
        }
    }

#else

    for (unsigned i = 3; i < n; i += 2) {
        if (0 == (n % i)) {
            return false;
        }
    }

#endif

    return true;
}

// Reference version for validation
static uint16_t NextPrime16_Ref(uint16_t n)
{
    if (n == 0 || n == 1) {
        return 1;
    }
    if (n == 2) {
        return 2;
    }

    if (n > 65521) {
        return 0;
    }

    while (!IsPrime(n))
    {
        ++n;
        TABLEGEN_DEBUG_ASSERT(n != 0);
    }
    return n;
}


/// Size of the kSieveTable in elements
static const unsigned kSieveTableSize = 2 * 3 * 5 * 7;

/// Sieve table for the next prime finding function below
static uint8_t kSieveTable[kSieveTableSize] = {
    1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    7, 6, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0,
    1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

static void Generate_kSieveTable()
{
    for (unsigned i = 0; i < kSieveTableSize; ++i)
    {
        unsigned next = i;
        for (unsigned j = i; j < kSieveTableSize; ++j)
        {
            if (j % 2 != 0 &&
                j % 3 != 0 &&
                j % 5 != 0 &&
                j % 7 != 0 &&
                j % 9 != 0)
            {
                next = j;
                break;
            }
        }

        kSieveTable[i] = (uint8_t)(next - i);
    }
}

static void Print_kSieveTable()
{
    cout << endl;
    cout << "static const unsigned kSieveTableSize = " << kSieveTableSize << ";" << endl;
    cout << "static const uint8_t kSieveTable[" << kSieveTableSize << "] = {" << endl;

    const unsigned modulus = 2 * 3 * 5;

    for (unsigned i = 0; i < kSieveTableSize; ++i)
    {
        if (i % modulus == 0) {
            cout << "    ";
        }

        cout << (int)kSieveTable[i] << ", ";

        if ((i + 1) % modulus == 0) {
            cout << endl;
        }
    }

    cout << "};" << endl;
    cout << endl;
}

/// List of primes under 256, starting at 11
static vector<uint8_t> kPrimesUnder256From11;
static unsigned kPrimesUnder256Count = 0;

static void Generate_PRIMES_UNDER_256()
{
    kPrimesUnder256From11.clear();

    for (unsigned i = 11; i < 256; ++i)
    {
        if (IsPrime(i)) {
            kPrimesUnder256From11.push_back((uint8_t)i);
        }
    }

    kPrimesUnder256Count = (unsigned)kPrimesUnder256From11.size();
}

static void Print_PRIMES_UNDER_256()
{
    cout << endl;
    cout << "static const unsigned kPrimesUnder256Count = " << kPrimesUnder256Count << ";" << endl;
    cout << "static const uint8_t kPrimesUnder256From11[" << kPrimesUnder256Count << "] = {" << endl;

    const unsigned modulus = 10;

    for (unsigned i = 0; i < kPrimesUnder256Count; ++i)
    {
        if (i % modulus == 0) {
            cout << "    ";
        }

        cout << (int)kPrimesUnder256From11[i] << ", ";

        if ((i + 1) % modulus == 0) {
            cout << endl;
        }
    }

    cout << "};" << endl;
    cout << endl;
}


static uint16_t NextPrime16(uint16_t n)
{
    // Handle small n
    switch (n)
    {
    case 0:
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
    const uint8_t * primes = &kPrimesUnder256From11[0];

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

static bool Validate_NextPrime16()
{
    for (unsigned x = 0; x < 65536; ++x)
    {
        unsigned y = NextPrime16_Ref((uint16_t)x);
        unsigned z = NextPrime16((uint16_t)x);

        if (y != z) {
            TABLEGEN_DEBUG_BREAK();
            return false;
        }
    }

    return true;
}

static void Benchmark_NextPrime16()
{
    // Collect many samples to account for noise
    static const unsigned kSampleCount = 500;
    std::vector<uint32_t> referenceDelay;
    std::vector<uint32_t> customDelay;
    referenceDelay.reserve(kSampleCount);
    customDelay.reserve(kSampleCount);

    // Need to run each algorithm many times per bin
    static const unsigned kRunsPerSample = 1000;

    for (unsigned sample = 1; sample <= kSampleCount; ++sample)
    {
        unsigned sum = 0;

        PCGRandom prng;

        const uint64_t t0 = GetTimeUsec();

        prng.Seed(sample);

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = (uint16_t)prng.Next();
            const unsigned x = NextPrime16(value);
            sum ^= x;
        }

        const uint64_t t1 = GetTimeUsec();

        prng.Seed(sample);

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = (uint16_t)prng.Next();
            const unsigned expected = NextPrime16_Ref(value);
            sum ^= expected;
        }

        const uint64_t t2 = GetTimeUsec();

        if (sum != 0)
        {
            cout << "Test failed!" << endl;
            TABLEGEN_DEBUG_BREAK();
        }

        if (sample > 10)
        {
            customDelay.push_back((uint32_t)(t1 - t0));
            referenceDelay.push_back((uint32_t)(t2 - t1));
        }
    }

    cout << "NextPrime16 timing results vs reference algorithm:" << endl;

    const uint32_t custom25 = CalculatePercentile(customDelay, 0.25f);
    const uint32_t custom50 = CalculatePercentile(customDelay, 0.5f);
    const uint32_t custom75 = CalculatePercentile(customDelay, 0.75f);

    cout << "* Custom algorithm: 25% latency = " << custom25 << " usec" << endl;
    cout << "* Custom algorithm: 50% latency = " << custom50 << " usec (median)" << endl;
    cout << "* Custom algorithm: 75% latency = " << custom75 << " usec" << endl;

    const uint32_t reference25 = CalculatePercentile(referenceDelay, 0.25f);
    const uint32_t reference50 = CalculatePercentile(referenceDelay, 0.5f);
    const uint32_t reference75 = CalculatePercentile(referenceDelay, 0.75f);

    cout << "* Reference algorithm: 25% latency = " << reference25 << " usec" << endl;
    cout << "* Reference algorithm: 50% latency = " << reference50 << " usec (median)" << endl;
    cout << "* Reference algorithm: 75% latency = " << reference75 << " usec" << endl;
    cout << endl;
}


//------------------------------------------------------------------------------
// Utility: Peeling Row Weight Generator function

/// Maximum columns per peel matrix row
static const unsigned kMaxPeelCount = 64;

/// Map from a 32-bit uniform random number to a row weight
/// Probability of weight i = (i-1)/i
/// e.g. P(2) = 1/2, P(3) = 2/3, ...
/// P(1) is chosen based on logic below.
static uint32_t kPeelCountDistribution[kMaxPeelCount] = {};

void Generate_kPeelCountDistribution()
{
    kPeelCountDistribution[0] = 0;

    for (unsigned i = 1; i < kMaxPeelCount; ++i)
    {
        uint64_t x = 0x100000000ULL;
        uint64_t y = (x * i) / (i + 1);
        kPeelCountDistribution[i] = (uint32_t)y - 1;
    }

    // Clip distribution tail at kMaxPeelCount
    kPeelCountDistribution[kMaxPeelCount - 1] = 0xffffffff;
}

void Print_kPeelCountDistribution()
{
    cout << endl;
    cout << "static const unsigned kMaxPeelCount = 64;" << endl;
    cout << "static const uint32_t kPeelCountDistribution[kMaxPeelCount] = {" << endl;

    const unsigned modulus = 8;

    for (unsigned i = 0; i < kMaxPeelCount; ++i)
    {
        if (i % modulus == 0) {
            cout << "    ";
        }

        cout << "0x" << hex << setfill('0') << setw(8) << (int)kPeelCountDistribution[i] << ", ";

        if ((i + 1) % modulus == 0) {
            cout << endl;
        }
    }

    cout << "};" << endl;
    cout << dec << endl;
}



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
static inline void IterateNextColumn(
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

static inline void IterateNextColumn_Ref(
    uint16_t &x,      ///< Current column number to mutate
    const uint16_t b, ///< Non-prime modulus for addition
    const uint16_t p, ///< Next prime above b (or = b if prime)
    const uint16_t a  ///< Number to add for Weyl generator
)
{
    x = (x + a) % p;

    // While the result is in the range [b, p):
    while (x >= b) {
        x = (x + a) % p;
    }
}


bool TestColumnCount(unsigned peel_column_count)
{
    const unsigned p = NextPrime16((uint16_t)peel_column_count);
    if (p <= 0) {
        return true;
    }

    for (int x = 0; x < (int)peel_column_count; ++x)
    {
        for (unsigned a = 1; a < peel_column_count; ++a)
        {
            uint16_t y = (uint16_t)x;
            IterateNextColumn(y, (uint16_t)peel_column_count, (uint16_t)p, (uint16_t)a);

            uint16_t z = (uint16_t)x;
            IterateNextColumn_Ref(z, (uint16_t)peel_column_count, (uint16_t)p, (uint16_t)a);

            if (y != z) {
                TABLEGEN_DEBUG_BREAK();
                return false;
            }
        }
    }

    return true;
}

bool Validate_IterateNextColumn()
{
    for (unsigned peel_column_count = 1; peel_column_count <= 1000; ++peel_column_count)
    {
        if (!TestColumnCount(peel_column_count)) {
            return false;
        }
    }

    return true;
}


static void Benchmark_IterateNextColumn()
{
    // Collect many samples to account for noise
    static const unsigned kSampleCount = 500;
    std::vector<uint32_t> referenceDelay;
    std::vector<uint32_t> customDelay;
    referenceDelay.reserve(kSampleCount);
    customDelay.reserve(kSampleCount);

    // Need to run each algorithm many times per bin
    static const unsigned kRunsPerSample = 100000;

    for (unsigned sample = 1; sample <= kSampleCount; ++sample)
    {
        unsigned sum = 0;

        PCGRandom prng;

        prng.Seed(sample);

        const unsigned columns = (prng.Next() % (65521 - 1)) + 2;
        const unsigned p = NextPrime16((uint16_t)columns);
        const unsigned x = prng.Next() % columns;
        const unsigned a = (prng.Next() % (columns - 1)) + 1;

        const uint64_t t0 = GetTimeUsec();

        uint16_t y = (uint16_t)x;

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            IterateNextColumn(y, (uint16_t)columns, (uint16_t)p, (uint16_t)a);

            sum ^= y;
        }

        const uint64_t t1 = GetTimeUsec();

        y = (uint16_t)x;

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            IterateNextColumn_Ref(y, (uint16_t)columns, (uint16_t)p, (uint16_t)a);

            sum ^= y;
        }

        const uint64_t t2 = GetTimeUsec();

        if (sum != 0)
        {
            cout << "Test failed!" << endl;
            TABLEGEN_DEBUG_BREAK();
        }

        if (sample > 10)
        {
            customDelay.push_back((uint32_t)(t1 - t0));
            referenceDelay.push_back((uint32_t)(t2 - t1));
        }
    }

    cout << "IterateNextColumn timing results vs reference algorithm:" << endl;

    const uint32_t custom25 = CalculatePercentile(customDelay, 0.25f);
    const uint32_t custom50 = CalculatePercentile(customDelay, 0.5f);
    const uint32_t custom75 = CalculatePercentile(customDelay, 0.75f);

    cout << "* Custom algorithm: 25% latency = " << custom25 << " usec" << endl;
    cout << "* Custom algorithm: 50% latency = " << custom50 << " usec (median)" << endl;
    cout << "* Custom algorithm: 75% latency = " << custom75 << " usec" << endl;

    const uint32_t reference25 = CalculatePercentile(referenceDelay, 0.25f);
    const uint32_t reference50 = CalculatePercentile(referenceDelay, 0.5f);
    const uint32_t reference75 = CalculatePercentile(referenceDelay, 0.75f);

    cout << "* Reference algorithm: 25% latency = " << reference25 << " usec" << endl;
    cout << "* Reference algorithm: 50% latency = " << reference50 << " usec (median)" << endl;
    cout << "* Reference algorithm: 75% latency = " << reference75 << " usec" << endl;
    cout << endl;
}


//------------------------------------------------------------------------------
// Utility: Deck Shuffling function

void ShuffleDeck16(
    PCGRandom &prng,
    uint16_t * deck,
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

// "Inside out" shuffle algorithm
void ShuffleDeck16_Ref(
    PCGRandom &prng,
    uint16_t * deck,
    const uint32_t count)
{
#if 1

    deck[0] = 0;

    for (unsigned i = 1; i < count; ++i)
    {
        unsigned j = prng.Next() % (i + 1);
        if (j != i) {
            deck[i] = deck[j];
        }
        deck[j] = (uint16_t)i;
    }

#else

    (void)prng;

    for (unsigned i = 0; i < count; ++i)
    {
        deck[i] = (uint16_t)i;
    }

    // This is even slower
    std::random_shuffle(&deck[0], &deck[count]);

#endif
}

bool Validate_ShuffleDeck16()
{
    std::vector<uint16_t> deck;
    deck.resize(65536);

    std::vector<bool> seen;

    for (unsigned i = 0; i < 100; ++i)
    {
        PCGRandom prng;

        prng.Seed(i);

        unsigned count = (prng.Next() % 65536) + 1;

        // Test some special cases
        if (i == 0) {
            count = 1;
        }
        else if (i == 1) {
            count = 2;
        }
        else if (i == 2) {
            count = 256;
        }

        ShuffleDeck16_Ref(prng, &deck[0], count);

        seen.assign(count, false);
        for (unsigned j = 0; j < count; ++j)
        {
            unsigned x = deck[j];
            if (seen[x]) {
                TABLEGEN_DEBUG_BREAK();
                return false;
            }
            seen[x] = true;
        }

        ShuffleDeck16(prng, &deck[0], count);

        seen.assign(count, false);
        for (unsigned j = 0; j < count; ++j)
        {
            unsigned x = deck[j];
            if (seen[x]) {
                TABLEGEN_DEBUG_BREAK();
                return false;
            }
            seen[x] = true;
        }
    }

    return true;
}


static void Benchmark_ShuffleDeck16()
{
    // Collect many samples to account for noise
    static const unsigned kSampleCount = 100;
    std::vector<uint32_t> referenceDelay;
    std::vector<uint32_t> customDelay;
    referenceDelay.reserve(kSampleCount);
    customDelay.reserve(kSampleCount);

    std::vector<uint16_t> deck;
    deck.resize(65536);

    // Need to run each algorithm many times per bin
    static const unsigned kRunsPerSample = 10;

    unsigned sum = 0;

    for (unsigned sample = 1; sample <= kSampleCount; ++sample)
    {
        PCGRandom prng;

        const uint64_t t0 = GetTimeUsec();

        prng.Seed(sample);

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            unsigned count = (prng.Next() % 65536) + 1;
            ShuffleDeck16(prng, &deck[0], count);

            for (unsigned i = 0; i < count; ++i) {
                // Make the sum somewhat order-dependent
                sum += deck[i] * (i + 1);
            }
        }

        const uint64_t t1 = GetTimeUsec();

        prng.Seed(sample);

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            unsigned count = (prng.Next() % 65536) + 1;
            ShuffleDeck16_Ref(prng, &deck[0], count);

            for (unsigned i = 0; i < count; ++i) {
                // Make the sum somewhat order-dependent
                sum += deck[i] * (i + 1);
            }
        }

        const uint64_t t2 = GetTimeUsec();

        if (sample > 10)
        {
            customDelay.push_back((uint32_t)(t1 - t0));
            referenceDelay.push_back((uint32_t)(t2 - t1));
        }
    }

    if (sum != 408201296)
    {
        cout << "Test failed!" << endl;
        TABLEGEN_DEBUG_BREAK();
    }

    cout << "ShuffleDeck16 timing results vs reference algorithm:" << endl;

    const uint32_t custom25 = CalculatePercentile(customDelay, 0.25f);
    const uint32_t custom50 = CalculatePercentile(customDelay, 0.5f);
    const uint32_t custom75 = CalculatePercentile(customDelay, 0.75f);

    cout << "* Custom algorithm: 25% latency = " << custom25 << " usec" << endl;
    cout << "* Custom algorithm: 50% latency = " << custom50 << " usec (median)" << endl;
    cout << "* Custom algorithm: 75% latency = " << custom75 << " usec" << endl;

    const uint32_t reference25 = CalculatePercentile(referenceDelay, 0.25f);
    const uint32_t reference50 = CalculatePercentile(referenceDelay, 0.5f);
    const uint32_t reference75 = CalculatePercentile(referenceDelay, 0.75f);

    cout << "* Reference algorithm: 25% latency = " << reference25 << " usec" << endl;
    cout << "* Reference algorithm: 50% latency = " << reference50 << " usec (median)" << endl;
    cout << "* Reference algorithm: 75% latency = " << reference75 << " usec" << endl;
    cout << endl;
}


//------------------------------------------------------------------------------
// Utility: GF(2) Invertible Matrix Generator function

static const unsigned kMatrixSeedCount = 512;

/**
    Sometimes it is helpful to be able to quickly generate a GF2 matrix
    that is invertible.  I guess.  Anyway, here's a lookup table of
    seeds that create invertible GF2 matrices and a function that will
    fill a bitfield with the matrix.
*/
static uint8_t kInvertibleMatrixSeeds[kMatrixSeedCount] = {
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

static inline uint64_t Random64(PCGRandom& prng)
{
    const uint32_t rv1 = prng.Next();
    const uint32_t rv2 = prng.Next();
    return ((uint64_t)rv2 << 32) | rv1;
}

static void AddInvertibleGF2Matrix(
    uint64_t * matrix, ///< Address of upper left word
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

#define CAT_ROL64(n, r) ( ((uint64_t)(n) << (r)) | ((uint64_t)(n) >> (64 - (r))) )

static bool IsFullRank(
    uint64_t * matrix, ///< Address of upper left word
    const unsigned offset, ///< Offset in bits to the first sum column
    const unsigned pitchWords, ///< Pitch of the matrix in words
    const unsigned n ///< Number of bits in the matrix to generate
)
{
    vector<uint16_t> pivot(n);

    // Initialize pivot array
    for (unsigned i = 0; i < n; ++i) {
        pivot[i] = (uint16_t)i;
    }

    uint64_t ge_mask = (uint64_t)1 << (offset % 64);

    const unsigned lastWrittenWord = ((offset % 64) + n - 1) / 64;

    // For each pivot to determine:
    for (unsigned i = 0; i < n; ++i)
    {
        const unsigned wordOffset_i = (offset + i) >> 6;

        bool found = false;

        // Find pivot
        for (unsigned j = i; j < n; ++j)
        {
            // Determine if the row contains the bit we want
            const unsigned ge_row_j = pivot[j];
            const uint64_t * ge_row = matrix + pitchWords * ge_row_j;

            // Unroll first word
            const uint64_t row0 = ge_row[wordOffset_i];

            // If the bit was found:
            if (0 != (row0 & ge_mask))
            {
                found = true;

                // Swap out the pivot index for this one
                if (i != j) {
                    const uint16_t temp = pivot[i];
                    pivot[i] = pivot[j];
                    pivot[j] = temp;
                }

                // For each remaining unused row:
                for (unsigned k = j + 1; k < n; ++k)
                {
                    // Determine if the row contains the bit we want
                    const unsigned ge_row_k = pivot[k];
                    uint64_t * rem_row = matrix + pitchWords * ge_row_k;

                    // If the bit was found:
                    if (0 != (rem_row[wordOffset_i] & ge_mask))
                    {
                        // Add the pivot row to eliminate the bit from this row, preserving previous bits
                        rem_row[wordOffset_i] ^= row0;

                        for (unsigned ii = wordOffset_i + 1; ii <= lastWrittenWord; ++ii) {
                            rem_row[ii] ^= ge_row[ii];
                        }
                    }
                }

                break;
            }
        }

        // If pivot could not be found:
        if (!found) {
            return false;
        }

        // Generate next mask
        ge_mask = CAT_ROL64(ge_mask, 1);
    }

    return true;
}

void Generate_kInvertibleMatrixSeeds()
{
    vector<uint64_t> matrix;

    kInvertibleMatrixSeeds[0] = 0;

    for (unsigned n = 1; n < kMatrixSeedCount; ++n)
    {
        unsigned pitchWords = (n + 63) / 64;
        const unsigned wordCount = n * pitchWords;
        matrix.resize(wordCount);

        bool found = false;

        for (unsigned k = 0; k < 256; ++k)
        {
            kInvertibleMatrixSeeds[n] = (uint8_t)k;

            memset(&matrix[0], 0, wordCount * sizeof(uint64_t));

            AddInvertibleGF2Matrix(&matrix[0], 0, pitchWords, n);

            if (IsFullRank(&matrix[0], 0, pitchWords, n)) {
                found = true;
                break;
            }
        }

        if (!found) {
            cout << "Failed " << n << endl;
            TABLEGEN_DEBUG_BREAK();
        }
    }
}

void Print_kInvertibleMatrixSeeds()
{
    cout << endl;
    cout << "static const unsigned kMatrixSeedCount = 512;" << endl;
    cout << "static const uint8_t kInvertibleMatrixSeeds[kMatrixSeedCount] = {" << endl;

    const unsigned modulus = 32;

    for (unsigned i = 0; i < kMatrixSeedCount; ++i)
    {
        if (i % modulus == 0) {
            cout << "    ";
        }

        cout << (int)kInvertibleMatrixSeeds[i] << ",";

        if ((i + 1) % modulus == 0) {
            cout << endl;
        }
    }

    cout << "};" << endl;
    cout << dec << endl;
}

bool Verify_AddInvertibleGF2Matrix()
{
    vector<uint64_t> matrix;

    // Go past 512 to check diagonal matrix
    for (unsigned n = 1; n < kMatrixSeedCount + 1; ++n)
    {
        for (unsigned offset = 0; offset < 64; ++offset)
        {
            unsigned pitchWords = (n + offset + 63) / 64;
            matrix.resize(n * pitchWords);

            memset(&matrix[0], 0, matrix.size() * sizeof(uint64_t));

            AddInvertibleGF2Matrix(&matrix[0], offset, pitchWords, n);

            // Verify that it does not overwrite columns outside of NxN region
            for (unsigned j = 0; j < n; ++j)
            {
                for (unsigned i = 0; i < offset; ++i)
                {
                    uint64_t w = matrix[(i >> 6) + j * pitchWords] & ((uint64_t)1 << (i & 63));
                    if (w != 0) {
                        TABLEGEN_DEBUG_BREAK();
                        return false;
                    }

                    unsigned testWord = ((n + offset + i) >> 6);
                    if (testWord < pitchWords) {
                        uint64_t u = matrix[((n + offset + i) >> 6) + j * pitchWords] & ((uint64_t)1 << ((n + offset + i) & 63));
                        if (u != 0) {
                            TABLEGEN_DEBUG_BREAK();
                            return false;
                        }
                    }
                }
            }

            if (!IsFullRank(&matrix[0], offset, pitchWords, n)) {
                TABLEGEN_DEBUG_BREAK();
                return false;
            }
        }
    }

    return true;
}


//------------------------------------------------------------------------------
// Dense point graph

static uint16_t LinearInterpolate(
    int N0, int N1,
    int Count0, int Count1,
    int N)
{
    SIAMESE_DEBUG_ASSERT(N >= N0 && N <= N1);

    const int numerator = (N - N0) * (Count1 - Count0);
    const int denominator = N1 - N0;

    const int count = Count0 + (unsigned)(numerator / denominator);

    SIAMESE_DEBUG_ASSERT(count > 0);

    return static_cast<uint16_t>(count);
}

/**
    Checking to make sure that our binary search works for all inputs and
    produces the same results as the reference version.
*/

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
    { 2618, 52 },
    { 2826, 58 },
    { 3725, 60 },
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
    { 23024, 147 },
    { 24119, 154 },
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
    { 46944, 353 },
    { 48421, 370 },
    { 49177, 373 },
    { 50725, 379 },
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

static uint16_t FindDenseCount_Ref(unsigned N)
{
    SIAMESE_DEBUG_ASSERT(N >= 2048 && N <= 64000);

    for (unsigned i = 0; i < kDensePointCount; ++i)
    {
        DensePoint lowPoint = kDensePoints[i];
        DensePoint highPoint = kDensePoints[i + 1];

        if (lowPoint.N <= N && highPoint.N >= N)
        {
            return LinearInterpolate(
                lowPoint.N,
                highPoint.N,
                lowPoint.DenseCount,
                highPoint.DenseCount,
                N);
        }
    }

    return 0;
}

static uint16_t FindDenseCount(unsigned N)
{
    unsigned low = 0;
    unsigned high = kDensePointCount - 1;

    for (;;)
    {
        const unsigned mid = (high + low) / 2;
        if (mid == low) {
            break;
        }

        DensePoint point = kDensePoints[mid];

        if (N > point.N) {
            low = mid;
        }
        else {
            high = mid;
        }
    }

    SIAMESE_DEBUG_ASSERT(low < kDensePointCount);
    DensePoint lowPoint = kDensePoints[low];
    SIAMESE_DEBUG_ASSERT(lowPoint.N <= N);
    SIAMESE_DEBUG_ASSERT(low + 1 < kDensePointCount);
    DensePoint highPoint = kDensePoints[low + 1];
    SIAMESE_DEBUG_ASSERT(highPoint.N >= N);

    return LinearInterpolate(
        lowPoint.N,
        highPoint.N,
        lowPoint.DenseCount,
        highPoint.DenseCount,
        N);
}

static bool Validate_DenseSeedCount()
{
    for (unsigned N = 2048; N < 64000; ++N)
    {
        const uint16_t ref = FindDenseCount_Ref(N);
        const uint16_t count = FindDenseCount(N);

        if (ref != count) {
            SIAMESE_DEBUG_BREAK();
            return false;
        }
    }

    return true;
}

static void Benchmark_DenseSeedCount()
{
    // Collect many samples to account for noise
    static const unsigned kSampleCount = 500;
    std::vector<uint32_t> referenceDelay;
    std::vector<uint32_t> customDelay;
    referenceDelay.reserve(kSampleCount);
    customDelay.reserve(kSampleCount);

    // Need to run each algorithm many times per bin
    static const unsigned kRunsPerSample = 10000;

    std::vector<uint16_t> sampleInputs;
    sampleInputs.resize(kRunsPerSample);

    for (unsigned sample = 1; sample <= kSampleCount; ++sample)
    {
        unsigned sum = 0;

        PCGRandom prng;

        prng.Seed(sample);

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = (uint16_t)(prng.Next() % (64000 - 2048 + 1)) + 2048;
            sampleInputs[run] = value;
        }

        const uint64_t t0 = GetTimeUsec();

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = sampleInputs[run];
            const unsigned x = FindDenseCount(value);
            sum ^= x;
        }

        const uint64_t t1 = GetTimeUsec();

        for (unsigned run = 0; run < kRunsPerSample; ++run)
        {
            const uint16_t value = sampleInputs[run];
            const unsigned expected = (uint16_t)FindDenseCount_Ref(value);
            sum ^= expected;
        }

        const uint64_t t2 = GetTimeUsec();

        if (sum != 0)
        {
            cout << "Test failed!" << endl;
            TABLEGEN_DEBUG_BREAK();
        }

        if (sample > 10)
        {
            customDelay.push_back((uint32_t)(t1 - t0));
            referenceDelay.push_back((uint32_t)(t2 - t1));
        }
    }

    cout << "FindDenseCount timing results vs reference algorithm:" << endl;

    const uint32_t custom25 = CalculatePercentile(customDelay, 0.25f);
    const uint32_t custom50 = CalculatePercentile(customDelay, 0.5f);
    const uint32_t custom75 = CalculatePercentile(customDelay, 0.75f);

    cout << "* Custom algorithm: 25% latency = " << custom25 << " usec" << endl;
    cout << "* Custom algorithm: 50% latency = " << custom50 << " usec (median)" << endl;
    cout << "* Custom algorithm: 75% latency = " << custom75 << " usec" << endl;

    const uint32_t reference25 = CalculatePercentile(referenceDelay, 0.25f);
    const uint32_t reference50 = CalculatePercentile(referenceDelay, 0.5f);
    const uint32_t reference75 = CalculatePercentile(referenceDelay, 0.75f);

    cout << "* Reference algorithm: 25% latency = " << reference25 << " usec" << endl;
    cout << "* Reference algorithm: 50% latency = " << reference50 << " usec (median)" << endl;
    cout << "* Reference algorithm: 75% latency = " << reference75 << " usec" << endl;
    cout << endl;
}


//------------------------------------------------------------------------------
// Entrypoint

int main()
{
    cout << "Wirehair Table Generator" << endl;

    bool enableBenchmarks = true;

#ifdef TABLEGEN_DEBUG
    cout << "WARNING: This is built in debug mode so benchmarks are disabled." << endl;
    enableBenchmarks = false;
#endif

    cout << endl;

    //--------------------------------------------------------------------------

    if (!Validate_DenseSeedCount()) {
        cout << "Validate_DenseSeedCount() : Tests failed" << endl;
        return -1;
    }
    else {
        cout << "FindDenseCount() : Tests successful!" << endl;
    }

    if (enableBenchmarks) {
        Benchmark_DenseSeedCount();
    }

    //--------------------------------------------------------------------------

    if (!Generate_HeavyRows()) {
        cout << "Generate_HeavyRows() : Tests failed" << endl;
        return -1;
    }

    //--------------------------------------------------------------------------

    if (enableBenchmarks) {
        Benchmark_PRNG();
    }

    //--------------------------------------------------------------------------

    Generate_kPeelCountDistribution();
    Print_kPeelCountDistribution();

    //--------------------------------------------------------------------------

    Generate_kInvertibleMatrixSeeds();
    if (!Verify_AddInvertibleGF2Matrix())
    {
        cout << "AddInvertibleGF2Matrix() : Tests failed" << endl;
        return -1;
    }
    else
    {
        cout << "AddInvertibleGF2Matrix() : Tests passed." << endl;
    }
    Print_kInvertibleMatrixSeeds();

    //--------------------------------------------------------------------------

    if (!Validate_ShuffleDeck16())
    {
        cout << "ShuffleDeck16() : Tests failed" << endl;
        return -1;
    }
    else
    {
        cout << "ShuffleDeck16() : Tests passed." << endl;
    }
    if (enableBenchmarks) {
        Benchmark_ShuffleDeck16();
    }

    //--------------------------------------------------------------------------

    Generate_kSquareRootTable();
    if (!Validate_FloorSquareRoot16())
    {
        cout << "FloorSquareRoot16() : Tests failed" << endl;
        return -1;
    }
    else
    {
        cout << "FloorSquareRoot16() : Tests passed." << endl;
    }
    if (enableBenchmarks) {
        Benchmark_FloorSquareRoot16();
    }
    Print_kSquareRootTable();

    //--------------------------------------------------------------------------

    Generate_PRIMES_UNDER_256();
    Generate_kSieveTable();
    if (!Validate_NextPrime16())
    {
        cout << "NextPrime16() : Tests failed" << endl;
        return -1;
    }
    else
    {
        cout << "NextPrime16() : Tests passed." << endl;
    }
    if (enableBenchmarks) {
        Benchmark_NextPrime16();
    }
    Print_PRIMES_UNDER_256();
    Print_kSieveTable();

    //--------------------------------------------------------------------------

    if (!Validate_IterateNextColumn())
    {
        cout << "IterateNextColumn() : Tests failed" << endl;
        return -1;
    }
    else
    {
        cout << "IterateNextColumn() : Tests passed." << endl;
    }
    if (enableBenchmarks) {
        Benchmark_IterateNextColumn();
    }

    //--------------------------------------------------------------------------

    cout << "Tests and table generation completed" << endl;

    TABLEGEN_DEBUG_BREAK();
    return 0;
}

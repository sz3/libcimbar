#include "../test/SiameseTools.h"

#include "../WirehairCodec.h"
#include "../WirehairTools.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <atomic>
using namespace std;

//#define ENABLE_FULL_SEARCH

#ifdef ENABLE_FULL_SEARCH
static const unsigned kDenseMin = 2;
static const unsigned kDenseMax = kDenseSeedCount - 1;
#else
static const unsigned kDenseMin = 52/4;
static const unsigned kDenseMax = 62/4;
#endif

static const float targetMsec = 5000; // msec

/**
    Wirehair seed selection methodology

    Overview:

    Wirehair's solver inverts a hybrid matrix with three types of rows:
    (1) Peel rows - Sparse binary mixing of recovery set.
    (2) Dense rows - Dense binary mixing of recovery set.
    (3) Heavy rows - Small GF(256) Cauchy matrix in the lower right.

    The encoder inverts the matrix to find the recovery set,
    and then it generates new random peel rows for the decoder.

    The decoder inverts the matrix to find the recovery set,
    and then it regenerates the original data (first N peel rows).


    Heavy Rows:

    The heavy rows make up a special 6x18 Cauchy matrix with the property
    that any disturbance from the binary rows above does not affect the
    100% invertibility of the matrix.  Since these rows are not affected
    by any of the rows above, we just pick one of these special matrices
    and use it for all values of N original blocks.

    The code to generate this is in HeavyRowGenerator.cpp


    Dense Rows:

    We have two knobs to tune for these:
    (1) The seed to generate the random rows.
    (2) The number of rows to generate.

    The seed seems somewhat arbitrary.  Some will be better than others,
    but on average I expected them to be pretty similar.  The number of
    rows to generate seemed more important.  I expected that increasing
    the number of dense rows past a certain point would stop helping.
    Also the number of dense rows needed would increase as N increases,
    and should not decrease.  This means that once the number of dense
    rows is determined, then the seed can be optimized further at the
    largest N that uses it, which should also help all the smaller N
    that use the same row count.  So we can use a small list of seeds
    for each of the dense row counts.

    For benchmarking different dense row counts, we would want to pick
    a bunch of random seeds and check invertibility rates.  There should
    be a knee where increasing the row count doesn't help.


    Peel Rows:

    These are for the most part arbitrary.  Optimizing them to be more
    robust to a small number of losses seems worthwhile.
*/

/**
    GenerateDenseSeeds.cpp generates the dense seeds.

    It picks random peel seeds, tries many dense seeds, and all dense counts.
*/

/**
    Some old notes: 

    Each element of the DENSE_SEEDS table represents the best
    PCGRandom PRNG seed to use to generate a Shuffle-2 Code
    for the dense rows that has "good" properties.

    Element 0 is for D = 14,
    Element 1 is for D = 18,
    Element 2 is for D = 22,
    and so on.

    If the dense matrix size is DxD, then when D Mod 4 = 0,
    a Shuffle-2 Code matrix is not invertible.  And when
    D Mod 4 = 2, then it is better than the random matrix
    for small D.  However for D > 22, even the best sizes
    are not as often invertible as the random matrix.

    However this is not a huge issue.  Skipping over a lot
    of details out of scope of this article, the way that
    the Shuffle-2 Codes are used is as follows:

    How the codes are actually used:

        (1) Several DxD random matrices are produced with
        Shuffle-2 Codes.  Call these matrices {R0, R1, R2, R3...}.

        (2) Only a few columns (M) are selected at random from
        these matrices to form a new matrix, called the GE matrix.
        M is slightly larger than the square root of the total
        number of columns across all random R# matrices.

        (3) The resulting matrix must be rank M or it leads to
        lower error correcting performance in Wirehair.

        (4) Furthermore, due to the other things going on around
        this algorithm, the bits in the GE matrix are somewhat
        randomly flipped after the first third of them.
        This makes it easier for the matrix to be full rank, but
        is also a challenge because it means that if the columns
        have low Hamming weight that they are in danger of all
        being flipped off.

        (5) I want to be able to use the same random-looking R#
        matrices for any given D and I want it to behave well.
        This allows me to use a short table of PRNG seeds for
        each value of D to generate a best-performing set of
        R# matrices.

    From the way the Shuffle-2 Code is used,
    some requirements are apparent:

        (1) Should be able to generate the R# matrices from a seed.

        (2) The average rank of randomly-selected columns should
        be high to satisfy the primary goal of the code.

        (2a) To achieve (2), the average Hamming distance between
        columns should be maximized.

        (2b) The minimum Hamming distance between columns is 2.

        (2c) Based on the empirical data from before, D is chosen
        so that D Mod 4 = 2.
        In practice D will be rounded up to the next "good" one.

        (3) The minimum Hamming weight of each column should be 3
        to avoid being flipped into oblivion.

    To find the best matrices, I tried all seeds from 0..65535
    (time permitting) and generated D of the DxD matrices.
    Then, I verified requirement (2b) and (3).  Of the remaining
    options, the one with the highest average rank was chosen.
    I am only interested in values of D between 14 and 486 for
    practical use.  Smaller D are special cases that do not need
    to be in the table, and larger D are unused.

    I found that (2b) is not possible to satisfy.
    The minimum Hamming distance will always be 1.

    Here's some example data for 22x22:

    Seed 4504 minimum Hamming distance of 1 and
    average = 14.4 and minimum Hamming weight of 13
        Rank 2 at 0.999
        Rank 4 at 0.986
        Rank 6 at 0.96
        Rank 8 at 0.92
        Rank 10 at 0.887
        Rank 12 at 0.84
        Rank 14 at 0.804
        Rank 16 at 0.722
        Rank 18 at 0.648
        Rank 20 at 0.504
        Rank 21 at 0.36
        Rank 22 at 0.174

    I revised the seed search to look at the best two seeds
    in terms of average Hamming distance, and picked the one
    that is more often invertible for rank D-4 through D-2.
    This comes from the fact that often times the average
    Hamming distance being higher doesn't always mean it is
    better.  The real test is how often it is full rank.

    I set a 4 minute timeout for the best seed search and let
    it run overnight.  The result is a small 118-element table
    that determines all of the unchanging dense rows in the
    Wirehair check matrix for N=2 up to N=64000, providing
    best performance for a given number of dense rows.

    Some other random thoughts:

    + The average rank of randomly selected columns drops off
    pretty sharply near D.  To achieve 90% average invertibility,
    the number of rows needs to roughly double and add one.
    Adding just one row puts it above normal random matrices for
    invertibility rate.

    + The seeds are not necessarily the best that could be found,
    since for each D, a range of N use that value of D, and this
    is much less than D*D -- it is up to 64,000 tops.
*/


//// Entrypoint

static const unsigned kDenseSeedCount = 100;

static uint8_t kDenseSeeds[kDenseSeedCount] = {
};

void FillSeeds()
{
    memcpy(kDenseSeeds, wirehair::kDenseSeeds, sizeof(kDenseSeeds));
}

std::vector<uint8_t> message;

static std::atomic<unsigned> FailedTrials(0);

static void RandomTrial(
    unsigned N,
    unsigned count,
    uint64_t seed,
    const uint16_t d_seed,
    unsigned trial)
{
    const uint16_t dense_count = (uint16_t)count;

    siamese::PCGRandom prng;
    prng.Seed(seed, trial);

    const uint16_t p_seed = (uint16_t)(prng.Next() ^ prng.Next());

    wirehair::Codec codec;

    // Override the seeds
    codec.OverrideSeeds(dense_count, p_seed, d_seed);

    // Initialize codec
    WirehairResult result = codec.InitializeEncoder(N, 1);

    // If initialization succeeded:
    if (result == Wirehair_Success) {
        // Feed message to codec
        result = codec.EncodeFeed(&message[0]);
    }

    if (result != Wirehair_Success) {
        FailedTrials++;
    }
}

int main()
{
    FillSeeds();

    const int gfInitResult = gf256_init();

    // If gf256 init failed:
    if (gfInitResult != 0)
    {
        cout << "GF256 init failed" << endl;
        return -1;
    }

    message.resize(64000);

    uint64_t seed = siamese::GetTimeUsec();

    for (unsigned i_dense_count = kDenseMin; i_dense_count <= kDenseMax; ++i_dense_count)
    {
        int N_found = -1;
        int count_found = -1;

        for (int N = 2048; N <= 64000; ++N)
        {
            const unsigned dense_count = wirehair::GetDenseCount(N);
            if (dense_count % 4 != 2)
            {
                cout << "FAIL" << endl;
                return -1;
            }
            const unsigned tableIndex = dense_count / 4;

            if (tableIndex == i_dense_count) {
                N_found = N;
                count_found = dense_count;
            }
        }

        if (N_found <= 0) {
            continue;
        }

        int NumTrials = 1000;

        {
            unsigned d_seed = 0;

            const int N = N_found;

            ++seed;

            FailedTrials = 0;

            uint64_t t0 = siamese::GetTimeUsec();

#pragma omp parallel for
            for (int trial = 0; trial < NumTrials; ++trial) {
                RandomTrial(N, count_found, seed, (uint16_t)d_seed, trial);
            }

            uint64_t t1 = siamese::GetTimeUsec();

            const float actualMsec = (t1 - t0) / 1000.f; // msec
            const float mult = targetMsec / actualMsec;
            NumTrials = (int)(NumTrials * mult);

            cout << "NumTrials=1000 took " << (t1 - t0) / 1000 << " msec.  Mult = " << mult << " -> NumTrials = " << NumTrials << endl;
        }

        int best_dense_seed = 0;
        int best_failures = 10000;

        for (unsigned d_seed = 0; d_seed < 256; ++d_seed)
        {
            const int N = N_found;

            ++seed;

            FailedTrials = 0;

            //uint64_t t0 = siamese::GetTimeUsec();

#pragma omp parallel for // num_threads(6)
            for (int trial = 0; trial < NumTrials; ++trial) {
                RandomTrial(N, count_found, seed, (uint16_t)d_seed, trial);
            }

            //uint64_t t1 = siamese::GetTimeUsec();
            //const float actualMsec = (t1 - t0) / 1000.f; // msec
            //cout << "In " << actualMsec << " msec" << endl;

            const int failures = FailedTrials;

            if (failures < best_failures)
            {
                best_dense_seed = d_seed;
                best_failures = failures;

                cout << "*** In progress N = " << N_found
                    << " dense_count = " << count_found << " : Picked seed = " << best_dense_seed
                    << " failures = " << best_failures << " avg fail = " << best_failures << endl;

                if (failures == 0) {
                    break;
                }
            }
        }

        if (best_failures >= 1000) {
            cout << "Unable to find any good dense seeds!" << endl;
        }

        cout << "Selected: N = " << N_found
            << " dense_count = " << count_found << " : Picked seed = " << best_dense_seed
            << " failures = " << best_failures << " avg fail = " << best_failures << endl;

        kDenseSeeds[i_dense_count] = (uint8_t)best_dense_seed;
    }

    cout << "static const unsigned kDenseSeedCount = " << kDenseSeedCount << ";" << endl;

    cout << "static uint8_t kDenseSeeds[kDenseSeedCount] = {" << endl;

    const unsigned modulus = 32;

    for (unsigned i = 0; i < kDenseSeedCount; ++i)
    {
        if (i % modulus == 0) {
            cout << "    ";
        }

        cout << (int)kDenseSeeds[i] << ",";

        if ((i + 1) % modulus == 0) {
            cout << endl;
        }
    }

    cout << "};" << endl;
    cout << dec << endl;

    return 0;
}

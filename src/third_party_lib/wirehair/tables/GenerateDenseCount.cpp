#include "../test/SiameseTools.h"

#include "../WirehairCodec.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <atomic>
using namespace std;


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
    GenerateDenseCount.cpp generates the dense counts.

    It picks random peel and dense seeds.  I found that about 500 trials
    and 10 failures is a good threshold for the "knee" of the curve for
    each N.  To make sure it found the knee I report the fourth dense
    count that is under 10.  Then I ran this a few times to produce
    the dense*.csv files in the repo.

    I approximated the shape of the curve with code in WirehairCodec.cpp
    in the ChooseSeeds() functions.
*/


//// Entrypoint

std::vector<uint8_t> message;

static std::atomic<unsigned> FailedTrials(0);

static void RandomTrial(
    unsigned N,
    unsigned count,
    uint64_t seed,
    unsigned trial)
{
    const uint16_t dense_count = (uint16_t)count;

    siamese::PCGRandom prng;
    prng.Seed(seed, trial);

    const uint16_t p_seed = (uint16_t)prng.Next();
    const uint16_t d_seed = (uint16_t)prng.Next();

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
    const int gfInitResult = gf256_init();

    // If gf256 init failed:
    if (gfInitResult != 0)
    {
        cout << "GF256 init failed" << endl;
        return -1;
    }

    message.resize(64000);

    uint64_t seed = siamese::GetTimeUsec();

    int lastBest = 0;

    static const int kTrials = 500;

    cout << "N\tDenseCount\tLowestFailures" << endl;

    for (int N = 2; N <= 64000;)
    {
        unsigned lowCount = 0;

        // Start close to the last best to save time
        int count = lastBest - 10;
        if (count < 1) {
            count = 1;
        }

        int lowestFailures = 100000;
        lastBest = 0;

        for (; count <= N; ++count)
        {
#if 0
            if (count % 4 != 2 && N > 32) {
                continue;
            }
#endif

            ++seed;
            FailedTrials = 0;

#pragma omp parallel for
            for (int trial = 0; trial < kTrials; ++trial) {
                RandomTrial(N, count, seed, trial);
            }

            const int failures = FailedTrials;
            //cout << " *** " << N << "\t" << count << "\t" << (failures / (float)kTrials) << endl;

            if (failures < lowestFailures)
            {
                lowestFailures = failures;
                lastBest = count;
            }

            if (failures <= 10)
            {
                if (++lowCount >= 4) {
                    break;
                }
            }
        }

        cout << N << "\t" << lastBest << "\t" << (lowestFailures / (float)kTrials) << endl;

        int scale = N / 64;
        if (scale < 1) {
            scale = 1;
        }
        N += scale;
    }

    return 0;
}

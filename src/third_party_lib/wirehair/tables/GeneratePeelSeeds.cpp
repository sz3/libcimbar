#include "../test/SiameseTools.h"

#include "../WirehairCodec.h"
#include "../WirehairTools.h"

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
    GeneratePeelSeeds.cpp generates the dense seeds.

    It tries all the peel seeds for subsets of data until it finds one that
    works best.
*/


//// Entrypoint

static bool SkipTuning = false;

static const int kMaxTuningTries = 8;

static const unsigned kPeelSeedSubdivisions = wirehair::kPeelSeedSubdivisions;

static uint8_t kPeelSeeds[kPeelSeedSubdivisions] = {
    // TODO
};

void FillSeeds()
{
    memcpy(kPeelSeeds, wirehair::kPeelSeeds, sizeof(kPeelSeeds));
}

static std::atomic<unsigned> FailedTrials(0);

uint8_t Message[64000];

static void QuickReject(
    unsigned N,
    const uint16_t p_seed)
{
    wirehair::Codec encoder;

    const uint16_t dense_count = wirehair::GetDenseCount(N);
    const uint16_t dense_seed = wirehair::GetDenseSeed(N, dense_count);

    // Override the seeds
    encoder.OverrideSeeds(dense_count, p_seed, dense_seed);

    // Initialize codec
    WirehairResult result = encoder.InitializeEncoder(N, 1);

    // If initialization succeeded:
    if (result == Wirehair_Success) {
        // Feed message to codec
        result = encoder.EncodeFeed(&Message[0]);
    }

    if (result != Wirehair_Success) {
        // Huge penalty
        ++FailedTrials;
    }
}

static void QuickPeelTest(
    unsigned N,
    const uint16_t p_seed,
    int trials)
{
    wirehair::Codec encoder, decoder;

    const uint16_t dense_count = wirehair::GetDenseCount(N);
    const uint16_t dense_seed = wirehair::GetDenseSeed(N, dense_count);

    // Override the seeds
    encoder.OverrideSeeds(dense_count, p_seed, dense_seed);
    decoder.OverrideSeeds(dense_count, p_seed, dense_seed);

    // Initialize codec
    WirehairResult result = encoder.InitializeEncoder(N, 1);

    // If initialization succeeded:
    if (result == Wirehair_Success) {
        // Feed message to codec
        result = encoder.EncodeFeed(&Message[0]);
    }

    if (result != Wirehair_Success) {
        // Huge penalty
        cout << "InitializeEncoder/EncodeFeed failed" << endl;
        FailedTrials += 10000;
        return;
    }

    result = decoder.InitializeDecoder(N, 1);

    if (result != Wirehair_Success) {
        cout << "InitializeDecoder failed" << endl;
        // Huge penalty
        FailedTrials += 10000;
        return;
    }

    unsigned added = 0;

    wirehair::PCGRandom prng;
    prng.Seed((uint64_t)N * p_seed, trials + p_seed);

    int lossCount = (N + 9) / 10;
    std::vector<uint16_t> losses;
    losses.resize(N);

    wirehair::ShuffleDeck16(prng, &losses[0], N);

    for (unsigned i = 0;; ++i)
    {
        bool match = false;
        for (int j = 0; j < lossCount; ++j) {
            if (losses[j] == i) {
                match = true;
                break;
            }
        }
        if (match) {
            continue;
        }

        ++added;

        uint8_t encodedData[1];

        const uint32_t encodedBytes = encoder.Encode(i, encodedData, 1);

        if (encodedBytes == 0)
        {
            cout << "Encode failed" << endl;
            // Huge penalty
            FailedTrials += 10000;
            return;
        }

        result = decoder.DecodeFeed(i, encodedData, encodedBytes);

        if (result != Wirehair_NeedMore)
        {
            if (result == Wirehair_Success)
            {
                if (added == N) {
                    // No penalty
                }
                else if (added == N + 1) {
                    FailedTrials++;
                }
                else if (added == N + 2) {
                    FailedTrials += 3;
                }
                else if (added >= N + 3) {
                    FailedTrials += (added - N) * 3;
                }
                break;
            }

            cout << "DecodeFeed failed" << endl;
            // Huge penalty
            FailedTrials += 10000;
            return;
        }

        if (added > N + 10) {
            cout << "added = " << added << " for " << N << endl;
            // Huge penalty
            FailedTrials += 10000;
            return;
        }
    }
}

static const int kTrials = 10;

int main()
{
    const int gfInitResult = gf256_init();

    FillSeeds();

    // If gf256 init failed:
    if (gfInitResult != 0)
    {
        cout << "GF256 init failed" << endl;
        return -1;
    }

    for (unsigned i = 0; i < 64000; ++i) {
        Message[i] = (uint8_t)i;
    }

    for (unsigned i = 2; i < kPeelSeedSubdivisions; ++i)
    {
        int best_peel_seed = 0;
        int best_failures = 10000;

        int tries = 0;

        for (unsigned p_seed = 0; p_seed < 256; ++p_seed)
        {
            FailedTrials = 0;

#pragma omp parallel for
            for (int N = 2048 + i; N <= 64000; N += kPeelSeedSubdivisions)
            {
                QuickReject(N, (uint16_t)p_seed);
            }

            if (FailedTrials > 0) {
                continue;
            }

            cout << "For subdivision " << i << " of " << kPeelSeedSubdivisions << " - testing seed " << p_seed << endl;

            if (SkipTuning)
            {
                best_peel_seed = p_seed;
                best_failures = 0;
                break;
            }

            FailedTrials = 0;

            for (int N = 2048 + i; N <= 64000; N += kPeelSeedSubdivisions)
            {
#pragma omp parallel for
                for (int trials = 0; trials < kTrials; ++trials) {
                    QuickPeelTest(N, (uint16_t)p_seed, trials);
                }
            }

            const int failures = FailedTrials;
            cout << "failures = " << failures << endl;

            if (failures < best_failures)
            {
                best_peel_seed = p_seed;
                best_failures = failures;

                cout << "*** subdivision = " << i << " : Picked seed = " << best_peel_seed << " failures = " << best_failures << endl;

                if (failures <= 0) {
                    break;
                }
            }

            ++tries;

            if (tries >= kMaxTuningTries) {
                break;
            }
        }

        cout << "subdivision = " << i << " : Picked seed = " << best_peel_seed << " failures = " << best_failures << endl;

        kPeelSeeds[i] = (uint8_t)best_peel_seed;
    }

    cout << "static const unsigned kPeelSeedSubdivisions = " << kPeelSeedSubdivisions << ";" << endl;

    cout << "const uint8_t kPeelSeeds[kPeelSeedSubdivisions] = {" << endl;

    const unsigned modulus = 32;

    for (unsigned i = 0; i < kPeelSeedSubdivisions; ++i)
    {
        if (i % modulus == 0) {
            cout << "    ";
        }

        cout << (int)kPeelSeeds[i] << ",";

        if ((i + 1) % modulus == 0) {
            cout << endl;
        }
    }

    cout << "};" << endl;


    return 0;
}

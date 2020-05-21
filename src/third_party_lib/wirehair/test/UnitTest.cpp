#include "../wirehair.h"

#include "SiameseTools.h"

#include <iostream>
#include <vector>
#include <atomic>
using namespace std;

#define ENABLE_OMP
#define ENABLE_PARTIAL_FINAL
#define ENABLE_ALL_RECOVERY_FINAL_BLOCK
#define BENCHMARK_SHORT_LIST

static const unsigned kMinN = 2;
static const unsigned kMaxN = 64000;
static const unsigned kMaxBlockBytes = 65;
static const unsigned kExtraWarnThreshold = 4;

static const uint8_t kPadChar = (uint8_t)0xee;

static void FillMessage(uint8_t* message, unsigned bytes, siamese::PCGRandom& prng)
{
    while (bytes >= 4)
    {
        uint32_t* words = reinterpret_cast<uint32_t*>(message);
        *words = prng.Next();
        message += 4;
        bytes -= 4;
    }

    if (bytes > 0)
    {
        uint32_t word = prng.Next();
        for (unsigned j = 0; j < bytes; ++j)
        {
            message[j] = (uint8_t)word;
            word >>= 8;
        }
    }
}

#pragma warning(disable: 4505)

static bool Test_EncoderProducesOriginals(
    WirehairCodec encoder,
    unsigned N,
    uint8_t* block,
    uint8_t* message,
    unsigned blockBytes,
    unsigned finalBytes)
{
    for (unsigned originalBlockId = 0; originalBlockId < N; ++originalBlockId)
    {
        if (originalBlockId == N - 1) {
            memset(block, kPadChar, blockBytes);
        }

        uint32_t bytesOut = 0;
        WirehairResult encodeResult = wirehair_encode(encoder, originalBlockId, &block[0], blockBytes, &bytesOut);
        if (encodeResult != Wirehair_Success)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "!!! Encode original failed for N = " << N << ", originalBlockId = " << originalBlockId << endl;
            return false;
        }

        if (originalBlockId == N - 1)
        {
            if (bytesOut != finalBytes)
            {
                SIAMESE_DEBUG_BREAK();
                cout << "!!! Encode original wrong length for N = " << N
                    << ", originalBlockId = " << originalBlockId
                    << ", bytesOut = " << bytesOut << ", finalBytes = " << finalBytes << endl;
                return false;
            }
            if (0 != memcmp(&message[0] + originalBlockId * blockBytes, &block[0], bytesOut))
            {
                SIAMESE_DEBUG_BREAK();
                cout << "!!! Encode original wrong data for N = " << N
                    << ", originalBlockId = " << originalBlockId
                    << ", bytesOut = " << bytesOut << ", finalBytes = " << finalBytes << endl;
                return false;
            }
            for (unsigned i = finalBytes; i < blockBytes; ++i)
            {
                if (block[i] != kPadChar)
                {
                    SIAMESE_DEBUG_BREAK();
                    cout << "!!! Encode original buffer overrun for N = " << N
                        << ", originalBlockId = " << originalBlockId
                        << ", bytesOut = " << bytesOut << ", finalBytes = " << finalBytes << endl;
                    return false;
                }
            }
        }
        else
        {
            if (bytesOut != blockBytes)
            {
                SIAMESE_DEBUG_BREAK();
                cout << "!!! Encode original wrong length for N = " << N
                    << ", originalBlockId = " << originalBlockId
                    << ", bytesOut = " << bytesOut << ", blockBytes = " << blockBytes << endl;
                return false;
            }
            if (0 != memcmp(&message[0] + originalBlockId * blockBytes, &block[0], bytesOut))
            {
                SIAMESE_DEBUG_BREAK();
                cout << "!!! Encode original wrong data for N = " << N
                    << ", originalBlockId = " << originalBlockId
                    << ", bytesOut = " << bytesOut << ", finalBytes = " << finalBytes << endl;
                return false;
            }
        }
    }

    return true;
}

static bool DoesCodecProduceOriginals(
    WirehairCodec decoder,
    unsigned N,
    unsigned blockBytes,
    unsigned finalBytes,
    const uint8_t* message,
    uint8_t* decodedMessage,
    unsigned messageBytes)
{
    // Check buffer overflow
    decodedMessage[messageBytes] = kPadChar;

    WirehairResult recoverResult = wirehair_recover(decoder, decodedMessage, messageBytes);

    if (recoverResult != Wirehair_Success)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "Failed to recover with original pieces for N = " << N << endl;
        return false;
    }

    if (0 != memcmp(message, decodedMessage, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "Failed to recover the data with original pieces for N = " << N << endl;
        return false;
    }

    if (decodedMessage[messageBytes] != kPadChar)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "Failed to recover buffer overrun with original pieces for N = " << N << endl;
        return false;
    }

    for (unsigned i = 0; i < N; ++i)
    {
        decodedMessage[finalBytes] = kPadChar;

        uint32_t recoveredBytes = 0;
        WirehairResult blockResult = wirehair_recover_block(decoder, i, decodedMessage, &recoveredBytes);

        if (blockResult != Wirehair_Success)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "Failed to wirehair_recover_block with original pieces for N = " << N << ", i = " << i << endl;
            return false;
        }

        if ((i == N - 1) && decodedMessage[finalBytes] != kPadChar)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "Failed to wirehair_recover_block memory corruption with original pieces for N = " << N << endl;
            return false;
        }

        const unsigned expectedBytes = (i == N - 1) ? finalBytes : blockBytes;

        if (0 != memcmp(message + i * blockBytes, decodedMessage, expectedBytes))
        {
            SIAMESE_DEBUG_BREAK();
            cout << "Failed to wirehair_recover_block the data with original pieces for N = " << N << endl;
            return false;
        }
    }

    return true;
}

static bool TestAllRecoveryData(
    WirehairCodec encoder,
    WirehairCodec decoder,
    unsigned N,
    unsigned blockBytes,
    unsigned finalBytes,
    const uint8_t* message,
    uint8_t* decodedMessage,
    unsigned messageBytes)
{
    unsigned needed = 0;

#ifdef ENABLE_ALL_RECOVERY_FINAL_BLOCK
    // Check the special case final block:
    {
        uint32_t Nm1Len = 0;
        WirehairResult encodeResult = wirehair_encode(encoder, N - 1, decodedMessage, blockBytes, &Nm1Len);

        if (encodeResult != Wirehair_Success)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_encode failed for N-1 and N = " << N << endl;
            return false;
        }

        if (Nm1Len != finalBytes)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_encode failed wrong len for N-1 and N = " << N << endl;
            return false;
        }

        WirehairResult decodeResult = wirehair_decode(decoder, N - 1, decodedMessage, Nm1Len);

        if (decodeResult != Wirehair_NeedMore)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_decode failed for N-1 and N = " << N << endl;
            return false;
        }

        ++needed;
    }
#endif

    for (unsigned blockId = N;; ++blockId)
    {
        ++needed;

        uint32_t writeLen = 0;
        WirehairResult encodeResult = wirehair_encode(encoder, blockId, decodedMessage, blockBytes, &writeLen);

        if (encodeResult != Wirehair_Success)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_encode failed for N = " << N << endl;
            return false;
        }

        if (writeLen != blockBytes)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_encode failed wrong len for N-1 and N = " << N << endl;
            return false;
        }

        WirehairResult decodeResult = wirehair_decode(decoder, blockId, decodedMessage, writeLen);

        if (decodeResult != Wirehair_NeedMore)
        {
            if (decodeResult == Wirehair_Success) {
                break;
            }

            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_decode failed for " << blockId << " and N = " << N << endl;
            return false;
        }
    }

    if (needed >= N + kExtraWarnThreshold) {
        //SIAMESE_DEBUG_BREAK();
        cout << "TestAllRecoveryData: Too much overhead: " << (needed - N) << " extra for N=" << N << " Seed=n/a BlockBytes=" << blockBytes << endl;
        //return false;
    }

    if (!DoesCodecProduceOriginals(decoder, N, blockBytes, finalBytes, message, decodedMessage, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!! Decoder2 failed to produce originals" << endl;
        return false;
    }

    return true;
}

// Verify that decoder can recover when all input data is available
// ..and that it can turn into an encoder that reproduces all the original data
// ..and that a decoder fed with recovery data produced by the re-encoder will decode
static bool Test_DecodeAllOriginal(
    unsigned N,
    unsigned blockBytes,
    unsigned finalBytes,
    const uint8_t* message,
    uint8_t* decodedMessage,
    unsigned messageBytes)
{
    WirehairCodec decoder = wirehair_decoder_create(nullptr, messageBytes, blockBytes);
    if (!decoder)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Failed to create decoder for N = " << N << ", blockBytes = " << blockBytes << endl;
        return false;
    }

    // Feed it all original data (out of order)

    for (int originalBlockId = N - 1; originalBlockId >= 0; --originalBlockId)
    {
        const uint8_t* originalBlock = &message[0] + originalBlockId * blockBytes;
        const unsigned originalBlockBytes = ((unsigned)originalBlockId == N - 1) ? finalBytes : blockBytes;

        WirehairResult decodeResult = wirehair_decode(decoder, originalBlockId, originalBlock, originalBlockBytes);

        // If this is the last one:
        if (originalBlockId == 0)
        {
            if (decodeResult != Wirehair_Success)
            {
                SIAMESE_DEBUG_BREAK();
                cout << "Failed to decode with original pieces for N = " << N << ", originalBlockId = " << originalBlockId << endl;
                return false;
            }
        }
        else if (decodeResult != Wirehair_NeedMore)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "Failed to decode midway with original pieces for N = " << N << ", originalBlockId = " << originalBlockId << endl;
            return false;
        }
    }

    if (!DoesCodecProduceOriginals(decoder, N, blockBytes, finalBytes, message, decodedMessage, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!! Decoder failed to produce originals" << endl;
        return false;
    }

    // ..and that it can turn into an encoder that reproduces all the original data
    WirehairResult becomeResult = wirehair_decoder_becomes_encoder(decoder);

    if (becomeResult != Wirehair_Success)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "Failed to wirehair_decoder_becomes_encoder with original pieces for N = " << N << endl;
        return false;
    }

    // ..and that a decoder fed with recovery data produced by the re-encoder will decode

    WirehairCodec decoder2 = wirehair_decoder_create(nullptr, messageBytes, blockBytes);
    if (!decoder2)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Failed to create decoder2 for N = " << N << ", blockBytes = " << blockBytes << endl;
        return false;
    }

    if (!TestAllRecoveryData(decoder, decoder2, N, blockBytes, finalBytes, message, decodedMessage, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "TestAllRecoveryData failed for N = " << N << endl;
        return false;
    }

    wirehair_free(decoder2);
    wirehair_free(decoder);

    return true;
}

// Verify that decoder can recover when no input data is available
// ..and that it can turn into an encoder that reproduces all the original data
// ..and that a decoder fed with recovery data produced by the re-encoder will decode
static bool Test_DecodeAllRecovery(
    unsigned N,
    unsigned blockBytes,
    unsigned finalBytes,
    const uint8_t* message,
    uint8_t* decodedMessage,
    unsigned messageBytes)
{
    WirehairCodec encoder = wirehair_encoder_create(nullptr, message, messageBytes, blockBytes);
    if (!encoder)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Failed to create encoder for N = " << N << ", blockBytes = " << blockBytes << endl;
        return false;
    }

    WirehairCodec decoder = wirehair_decoder_create(nullptr, messageBytes, blockBytes);
    if (!decoder)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Failed to create decoder for N = " << N << ", blockBytes = " << blockBytes << endl;
        return false;
    }

    if (!TestAllRecoveryData(encoder, decoder, N, blockBytes, finalBytes, message, decodedMessage, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "TestAllRecoveryData failed for N = " << N << endl;
        return false;
    }

    // ..and that it can turn into an encoder that reproduces all the original data
    WirehairResult becomeResult = wirehair_decoder_becomes_encoder(decoder);

    if (becomeResult != Wirehair_Success)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "Failed to wirehair_decoder_becomes_encoder with original pieces for N = " << N << endl;
        return false;
    }

    if (!DoesCodecProduceOriginals(decoder, N, blockBytes, finalBytes, message, decodedMessage, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!! Converted Decoder failed to produce originals" << endl;
        return false;
    }

    wirehair_free(decoder);
    wirehair_free(encoder);

    return true;
}

// Verify that decoder can recover when no input data is available
// Do not try to do decoder_becomes_encoder
static bool Test_DecodeRandomLosses(
    siamese::PCGRandom& prng,
    uint64_t seed,
    unsigned N,
    unsigned blockBytes,
    unsigned finalBytes,
    const uint8_t* message,
    uint8_t* decodedMessage,
    unsigned messageBytes)
{
    WirehairCodec encoder = wirehair_encoder_create(nullptr, message, messageBytes, blockBytes);
    if (!encoder)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Failed to create encoder for N = " << N << ", blockBytes = " << blockBytes << endl;
        return false;
    }

    WirehairCodec decoder = wirehair_decoder_create(nullptr, messageBytes, blockBytes);
    if (!decoder)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Failed to create decoder for N = " << N << ", blockBytes = " << blockBytes << endl;
        return false;
    }

    unsigned blockId = 0;
    unsigned needed = 0;
    for (;;)
    {
        blockId++;
        if (prng.Next() % 100 < 10) {
            continue;
        }

        ++needed;

        uint32_t writeLen = 0;
        WirehairResult encodeResult = wirehair_encode(encoder, blockId, decodedMessage, blockBytes, &writeLen);

        if (encodeResult != Wirehair_Success)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_encode failed for N = " << N << endl;
            return false;
        }

        if (writeLen != blockBytes)
        {
            if (blockId != N - 1 || writeLen != finalBytes)
            {
                SIAMESE_DEBUG_BREAK();
                cout << "wirehair_encode failed wrong len for " << blockId << " and N = " << N << endl;
                return false;
            }
        }

        WirehairResult decodeResult = wirehair_decode(decoder, blockId, decodedMessage, writeLen);

        if (decodeResult != Wirehair_NeedMore)
        {
            if (decodeResult == Wirehair_Success) {
                break;
            }

            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_decode failed for " << blockId << " and N = " << N << endl;
            return false;
        }
    }

    if (needed >= N + kExtraWarnThreshold) {
        //SIAMESE_DEBUG_BREAK();
        cout << "Test_DecodeRandomLosses: Too much overhead: " << (needed - N) << " extra for N=" << N << " Seed=" << seed <<" BlockBytes=" << blockBytes << endl;
        //return false;
    }

    if (!DoesCodecProduceOriginals(decoder, N, blockBytes, finalBytes, message, decodedMessage, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!! Decoder2 failed to produce originals" << endl;
        return false;
    }

    wirehair_free(decoder);
    wirehair_free(encoder);

    return true;
}

static atomic<bool> TestFailed(false);

static void TestN(uint64_t seed, int N, unsigned blockBytes)
{
    siamese::PCGRandom prng;
    prng.Seed(seed + N, blockBytes);

    vector<uint8_t> message(N * blockBytes + 1);
    vector<uint8_t> block(blockBytes);
    vector<uint8_t> decodedMessageVector(N * blockBytes + 1);

    // Final block is partial
#ifdef ENABLE_PARTIAL_FINAL
    const unsigned finalBytes = blockBytes == 1 ? 1 : (blockBytes - 1);
#else
    const unsigned finalBytes = blockBytes;
#endif
    const unsigned messageBytes = blockBytes * (N - 1) + finalBytes;

    uint8_t* decodedMessagePtr = &decodedMessageVector[0];

    // Set up a random message
    FillMessage(&message[0], messageBytes, prng);

    WirehairCodec encoder = wirehair_encoder_create(nullptr, &message[0], messageBytes, blockBytes);
    if (!encoder)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Failed to create encoder for N = " << N << ", blockBytes = " << blockBytes << endl;
        TestFailed = true;
        return;
    }

    // Verify that encoder produces original blocks

    if (!Test_EncoderProducesOriginals(encoder, N, &block[0], &message[0], blockBytes, finalBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Test_EncoderProducesOriginals failed" << endl;
        TestFailed = true;
        return;
    }

    // Decoder checks:

    if (!Test_DecodeAllOriginal(N, blockBytes, finalBytes, &message[0], decodedMessagePtr, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Test_DecodeAllOriginal failed" << endl;
        TestFailed = true;
        return;
    }

    if (!Test_DecodeAllRecovery(N, blockBytes, finalBytes, &message[0], decodedMessagePtr, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Test_DecodeAllRecovery failed" << endl;
        TestFailed = true;
        return;
    }

    if (!Test_DecodeRandomLosses(prng, seed, N, blockBytes, finalBytes, &message[0], decodedMessagePtr, messageBytes))
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Test_DecodeAllRecovery failed" << endl;
        TestFailed = true;
        return;
    }

    wirehair_free(encoder);
}


static bool ReadmeExample()
{
    // Size of packets to produce
    static const int kPacketSize = 1400;

    // Note: Does not need to be an even multiple of packet size or 16 etc
    static const int kMessageBytes = 1000 * 1000 + 333;

    vector<uint8_t> message(kMessageBytes);

    // Fill message contents
    memset(&message[0], 1, message.size());

    // Create encoder
    WirehairCodec encoder = wirehair_encoder_create(nullptr, &message[0], kMessageBytes, kPacketSize);
    if (!encoder)
    {
        cout << "!!! Failed to create encoder" << endl;
        return false;
    }

    // Create decoder
    WirehairCodec decoder = wirehair_decoder_create(nullptr, kMessageBytes, kPacketSize);
    if (!decoder)
    {
        // Free memory for encoder
        wirehair_free(encoder);

        cout << "!!! Failed to create decoder" << endl;
        return false;
    }

    unsigned blockId = 0, needed = 0;

    for (;;)
    {
        // Select which block to encode.
        // Note: First N blocks are the original data, so it's possible to start
        // sending data while wirehair_encoder_create() is getting started.
        blockId++;

        // Simulate 10% packetloss
        if (blockId % 10 == 0) {
            continue;
        }

        // Keep track of how many pieces were needed
        ++needed;

        vector<uint8_t> block(kPacketSize);

        // Encode a packet
        uint32_t writeLen = 0;
        WirehairResult encodeResult = wirehair_encode(
            encoder, // Encoder object
            blockId, // ID of block to generate
            &block[0], // Output buffer
            kPacketSize, // Output buffer size
            &writeLen); // Returned block length

        if (encodeResult != Wirehair_Success)
        {
            cout << "wirehair_encode failed: " << encodeResult << endl;
            return false;
        }

        // Attempt decode
        WirehairResult decodeResult = wirehair_decode(
            decoder, // Decoder object
            blockId, // ID of block that was encoded
            &block[0], // Input block
            writeLen); // Block length

        // If decoder returns success:
        if (decodeResult == Wirehair_Success) {
            // Decoder has enough data to recover now
            break;
        }

        if (decodeResult != Wirehair_NeedMore)
        {
            cout << "wirehair_decode failed: " << decodeResult << endl;
            return false;
        }
    }

    vector<uint8_t> decoded(kMessageBytes);

    // Recover original data on decoder side
    WirehairResult decodeResult = wirehair_recover(
        decoder,
        &decoded[0],
        kMessageBytes);

    if (decodeResult != Wirehair_Success)
    {
        cout << "wirehair_recover failed: " << decodeResult << endl;
        return false;
    }

    // Free memory for encoder and decoder
    wirehair_free(encoder);
    wirehair_free(decoder);

    return true;
}

static bool Benchmark(unsigned N, unsigned packetBytes, unsigned trials)
{
    siamese::PCGRandom prng;
    prng.Seed(N, packetBytes);

    const unsigned kBlockBytes = packetBytes;
    const unsigned kMessageBytes = N * kBlockBytes;

    vector<uint8_t> message(kMessageBytes);
    vector<uint8_t> block(kBlockBytes);
    vector<uint8_t> decoded(kMessageBytes);

    memset(&message[0], 6, kMessageBytes);

    uint64_t encode_create_sum = 0;
    uint64_t runs = 0;
    uint64_t extra_sum = 0;
    uint64_t encode_sum = 0;
    uint64_t decode_sum = 0;
    uint64_t packets = 0;
    uint64_t recover_sum = 0;

    for (unsigned trial = 0; trial < trials; ++trial)
    {
        ++runs;

        uint64_t t0 = siamese::GetTimeUsec();

        WirehairCodec encoder = wirehair_encoder_create(nullptr, &message[0], kMessageBytes, kBlockBytes);
        if (!encoder)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "!!! Failed to create encoder" << endl;
            return false;
        }

        uint64_t t1 = siamese::GetTimeUsec();

        WirehairCodec decoder = wirehair_decoder_create(nullptr, kMessageBytes, kBlockBytes);
        if (!decoder)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "!!! Failed to create decoder" << endl;
            return false;
        }

        encode_create_sum += t1 - t0;

        unsigned blockId = 0;
        unsigned needed = 0;
        for (;;)
        {
            blockId++;

            // Introduce about 30% loss to the data
            if (prng.Next() % 100 < 30) {
                continue;
            }

            ++needed;

            uint64_t t3 = siamese::GetTimeUsec();

            uint32_t writeLen = 0;
            WirehairResult encodeResult = wirehair_encode(
                encoder,
                blockId,
                &block[0],
                kBlockBytes,
                &writeLen);
            if (encodeResult != Wirehair_Success)
            {
                SIAMESE_DEBUG_BREAK();
                cout << "wirehair_encode failed" << endl;
                return false;
            }

            uint64_t t4 = siamese::GetTimeUsec();

            WirehairResult decodeResult = wirehair_decode(decoder, blockId, &block[0], writeLen);

            uint64_t t5 = siamese::GetTimeUsec();

            encode_sum += t4 - t3;
            decode_sum += t5 - t4;
            ++packets;

            if (decodeResult != Wirehair_NeedMore)
            {
                if (decodeResult == Wirehair_Success) {
                    break;
                }

                SIAMESE_DEBUG_BREAK();
                cout << "wirehair_decode failed for " << blockId << " and N = " << N << endl;
                return false;
            }
        }

        SIAMESE_DEBUG_ASSERT(needed >= N);
        extra_sum += needed - N;

        uint64_t t7 = siamese::GetTimeUsec();

        WirehairResult recoverResult = wirehair_recover(decoder, &decoded[0], kMessageBytes);

        uint64_t t8 = siamese::GetTimeUsec();

        if (recoverResult != Wirehair_Success)
        {
            SIAMESE_DEBUG_BREAK();
            cout << "wirehair_recover failed" << endl;
            return false;
        }

        recover_sum += t8 - t7;

        wirehair_free(decoder);
        wirehair_free(encoder);
    }

    cout << "For N = " << N << " packets of " << packetBytes << " bytes:" << endl;

    uint64_t avg_encode_create_usec = encode_create_sum / runs;

    float encode_create_MBPS = 0.f;
    if (avg_encode_create_usec > 0) {
        encode_create_MBPS = kMessageBytes / (float)avg_encode_create_usec;
    }

    cout << "+ Average wirehair_encoder_create() time: " << avg_encode_create_usec << " usec (" << encode_create_MBPS << " MBPS)" << endl;
    // wirehair_decoder_create() is super fast.  So is the decoder_becomes_encoder() most of the time

    uint64_t avg_encode_usec = encode_sum / packets;
    uint64_t avg_decode_usec = decode_sum / packets;

    float encode_MBPS = 0.f, decode_MBPS = 0.f;
    if (encode_sum > 0) {
        encode_MBPS = (packets * kBlockBytes) / (float)encode_sum;
    }
    if (decode_sum > 0) {
        decode_MBPS = (packets * kBlockBytes) / (float)decode_sum;
    }

    cout << "+ Average wirehair_encode() time: " << avg_encode_usec << " usec (" << encode_MBPS << " MBPS)" << endl;
    cout << "+ Average wirehair_decode() time: " << avg_decode_usec << " usec (" << decode_MBPS << " MBPS)" << endl;

    float avg_overhead_pieces = extra_sum / (float)runs;

    cout << "+ Average overhead piece count beyond N = " << avg_overhead_pieces << endl;

    uint64_t avg_recover_usec = recover_sum / runs;

    float recover_MBPS = 0.f;
    if (recover_sum > 0) {
        recover_MBPS = (runs * kMessageBytes) / (float)recover_sum;
    }

    cout << "+ Average wirehair_recover() time: " << avg_recover_usec << " usec (" << recover_MBPS << " MBPS)" << endl;

    return true;
}

static const unsigned kBenchmarkNList[] = {
    12,
    32,
    102,
    134,
    169,
    201,
    294,
    359,
    413,
    770,
    1000
};

int main()
{
    const WirehairResult initResult = wirehair_init();

    if (initResult != Wirehair_Success)
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Wirehair initialization failed: " << initResult << endl;
        return -1;
    }

    if (!ReadmeExample())
    {
        SIAMESE_DEBUG_BREAK();
        cout << "!!! Example usage failed" << endl;
        return -2;
    }

#ifdef BENCHMARK_SHORT_LIST
    for (unsigned i = 0; i < sizeof(kBenchmarkNList) / sizeof(kBenchmarkNList[0]); ++i)
    {
        const unsigned N = kBenchmarkNList[i];
#else
    for (unsigned N = 2; N < 64000; N *= 2)
    {
#endif
        const unsigned kPacketBytes = 1300;
        const unsigned kBenchTrials = 2000;

        if (!Benchmark(N, kPacketBytes, kBenchTrials))
        {
            SIAMESE_DEBUG_BREAK();
            cout << "!!! Benchmark failed" << endl;
            return -3;
        }
    }

    cout << "Wirehair Unit Test" << endl;

    uint64_t seed = siamese::GetTimeUsec();

    cout << "Start seed = " << seed << endl;

    for (unsigned N = kMinN; N <= kMaxN; ++N)
    {
#ifdef ENABLE_OMP
#pragma omp parallel for
#endif
        for (int blockBytes = 1; blockBytes <= kMaxBlockBytes; ++blockBytes) {
            TestN(seed, N, blockBytes);
        }

        if (TestFailed) {
            cout << "A test failed for N = " << N << endl;
            return -4;
        }

        //cout << "Test passed for N = " << N << endl;
    }

    return 0;
}

# Wirehair
## Fast and Portable Fountain Codes in C

Wirehair produces a stream of error correction blocks from a data source
using an erasure code.  When enough of these blocks are received,
the original data can be recovered.

As compared to other similar libraries, an unlimited number of error
correction blocks can be produced, and much larger block counts are supported.
Furthermore, it gets slower as O(N) in the amount of input data rather
than O(N Log N) like the Leopard block code or O(N^2) like the Fecal fountain code,
so it is well-suited for large data.

This is not an ideal MDS code, so sometimes it will fail to recover N
original data packets from N symbol packets.  It may take N + 1 or N + 2 or more.
On average it takes about N + 0.02 packets to recover.  Overall the overhead
from the code inefficiency is low, compared to LDPC and many other fountain codes.

A simple C API is provided to make it easy to incorporate into existing
projects.  No external dependencies are required.


##### Building: Quick Setup

The source code in this folder (gf256 and wirehair code) can be incorporated
into your project without any other external dependencies.

To build the software in this repo:

On Windows, make sure CMake and Git Bash are installed.  Open up git bash and then:

~~~
git clone git@github.com:catid/wirehair.git
cd wirehair
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
ls
explorer .
~~~

Then you can use Visual Studio Community Edition to open up the `wirehair.sln` file and build the software.


#### Example Usage

Here's an example program using Wirehair.  It's included in the UnitTest project and demonstrates both the sender and receiver, which are normally separate programs.  For example the data sender might be a file server and the data receiver might be downloading a file from the sender.

~~~
#include <wirehair/wirehair.h>

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
...
~~~


#### Benchmarks

Some quick comments:

Benchmarks on my PC do not mean a whole lot.  Right now it's clocked at 3 GHz and has Turbo Boost on, etc.
To run the test yourself just build and run the UnitTest project in Release mode.

For small values of N < 128 or so this is a pretty inefficient codec compared to the Fecal codec.  Fecal is also a fountain code but is limited to repairing a small number of failures or small input block count.

~~~
For N = 2 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 11 usec (236.364 MBPS)
+ Average wirehair_encode() time: 0 usec (7435.7 MBPS)
+ Average wirehair_decode() time: 2 usec (476.205 MBPS)
+ Average overhead piece count beyond N = 0.0105
+ Average wirehair_recover() time: 0 usec (9319 MBPS)

For N = 4 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 8 usec (650 MBPS)
+ Average wirehair_encode() time: 0 usec (8353.43 MBPS)
+ Average wirehair_decode() time: 1 usec (695.102 MBPS)
+ Average overhead piece count beyond N = 0.0225
+ Average wirehair_recover() time: 0 usec (11219 MBPS)

For N = 8 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 13 usec (800 MBPS)
+ Average wirehair_encode() time: 0 usec (7916.2 MBPS)
+ Average wirehair_decode() time: 1 usec (704.359 MBPS)
+ Average overhead piece count beyond N = 0.0045
+ Average wirehair_recover() time: 1 usec (8973.25 MBPS)

For N = 16 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 27 usec (770.37 MBPS)
+ Average wirehair_encode() time: 0 usec (7993.4 MBPS)
+ Average wirehair_decode() time: 1 usec (707.211 MBPS)
+ Average overhead piece count beyond N = 0.036
+ Average wirehair_recover() time: 2 usec (9116.81 MBPS)

For N = 32 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 41 usec (1014.63 MBPS)
+ Average wirehair_encode() time: 0 usec (7062.93 MBPS)
+ Average wirehair_decode() time: 1 usec (908.097 MBPS)
+ Average overhead piece count beyond N = 0.0195
+ Average wirehair_recover() time: 5 usec (8057.33 MBPS)

For N = 64 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 81 usec (1027.16 MBPS)
+ Average wirehair_encode() time: 0 usec (7159.51 MBPS)
+ Average wirehair_decode() time: 1 usec (1033.95 MBPS)
+ Average overhead piece count beyond N = 0.017
+ Average wirehair_recover() time: 10 usec (7640.74 MBPS)

For N = 128 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 192 usec (866.667 MBPS)
+ Average wirehair_encode() time: 0 usec (5662.07 MBPS)
+ Average wirehair_decode() time: 1 usec (870.14 MBPS)
+ Average overhead piece count beyond N = 0.015
+ Average wirehair_recover() time: 25 usec (6419.38 MBPS)

For N = 256 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 319 usec (1043.26 MBPS)
+ Average wirehair_encode() time: 0 usec (6333.2 MBPS)
+ Average wirehair_decode() time: 1 usec (1018.77 MBPS)
+ Average overhead piece count beyond N = 0.022
+ Average wirehair_recover() time: 50 usec (6602.26 MBPS)

For N = 512 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 670 usec (993.433 MBPS)
+ Average wirehair_encode() time: 0 usec (6483.91 MBPS)
+ Average wirehair_decode() time: 1 usec (1028.85 MBPS)
+ Average overhead piece count beyond N = 0.022
+ Average wirehair_recover() time: 100 usec (6600.1 MBPS)

For N = 1024 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 1697 usec (784.443 MBPS)
+ Average wirehair_encode() time: 0 usec (5309.05 MBPS)
+ Average wirehair_decode() time: 1 usec (671.005 MBPS)
+ Average overhead piece count beyond N = 0.022
+ Average wirehair_recover() time: 207 usec (6404.05 MBPS)

For N = 2048 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 3227 usec (825.039 MBPS)
+ Average wirehair_encode() time: 0 usec (5202.3 MBPS)
+ Average wirehair_decode() time: 1 usec (683.141 MBPS)
+ Average overhead piece count beyond N = 0.021
+ Average wirehair_recover() time: 441 usec (6026.08 MBPS)

For N = 4096 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 7614 usec (699.343 MBPS)
+ Average wirehair_encode() time: 0 usec (4334.08 MBPS)
+ Average wirehair_decode() time: 2 usec (577.674 MBPS)
+ Average overhead piece count beyond N = 0.0215
+ Average wirehair_recover() time: 1208 usec (4405.65 MBPS)

For N = 8192 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 17208 usec (618.875 MBPS)
+ Average wirehair_encode() time: 0 usec (3277.17 MBPS)
+ Average wirehair_decode() time: 2 usec (521.665 MBPS)
+ Average overhead piece count beyond N = 0.075
+ Average wirehair_recover() time: 2916 usec (3651.35 MBPS)

For N = 16384 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 42512 usec (501.016 MBPS)
+ Average wirehair_encode() time: 0 usec (2646.89 MBPS)
+ Average wirehair_decode() time: 2 usec (435.173 MBPS)
+ Average overhead piece count beyond N = 0.015
+ Average wirehair_recover() time: 7282 usec (2924.63 MBPS)

For N = 32768 packets of 1300 bytes:
+ Average wirehair_encoder_create() time: 111287 usec (382.78 MBPS)
+ Average wirehair_encode() time: 0 usec (2378.29 MBPS)
+ Average wirehair_decode() time: 3 usec (342.556 MBPS)
+ Average overhead piece count beyond N = 0.0195
+ Average wirehair_recover() time: 16326 usec (2609.23 MBPS)
~~~


#### Credits

Software by Christopher A. Taylor <mrcatid@gmail.com>

Please reach out if you need support or would like to collaborate on a project.

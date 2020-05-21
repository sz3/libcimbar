#
# Copyleft (c) 2019 Daniel Norte de Moraes <danielcheagle@gmail.com>.
#
# * This code is hereby placed in the public domain.
# *
# * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
# * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
# * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# An working example in python using the shared lib libwirehair-shared.so .
# adjust it for your actual lib and remenber you can do much more. Enjoy!!

import ctypes

# Success code
Wirehair_Success = 0

# More data is needed to decode.  This is normal and does not indicate a failure
Wirehair_NeedMore = 1

# Other values are failure codes:

# A function parameter was invalid
Wirehair_InvalidInput = 2

# Encoder needs a better dense seed
Wirehair_BadDenseSeed = 3

# Encoder needs a better peel seed
Wirehair_BadPeelSeed = 4

# N = ceil(messageBytes / blockBytes) is too small.
# Try reducing block_size or use a larger message
Wirehair_BadInput_SmallN = 5

# N = ceil(messageBytes / blockBytes) is too large.
# Try increasing block_size or use a smaller message
Wirehair_BadInput_LargeN = 6

# Not enough extra rows to solve it, must give up
Wirehair_ExtraInsufficient = 7

# An error occurred during the request
Wirehair_Error = 8

# Out of memory
Wirehair_OOM = 9

# Platform is not supported yet
Wirehair_UnsupportedPlatform = 10

WirehairResult_Count = 11  # /* for asserts */

WirehairResult_Padding = 0x7fffffff  # /* int32_t padding */

wirehair = ctypes.CDLL("libwirehair-shared.so")  # MSWindows: just remove ".so" part to use DLL

KPacketSize = ctypes.c_int(32)  # this can be extremaly large 1400 or more! :-)
Message_tmp = b'A working example. this need be minimum o 2*KPacketSize; because this I in filling ' \
              b'more and more words just' \
              b'by the sake of filling... :-)  the real data can be and will be different :-) '

Message = (ctypes.c_uint8 * len(Message_tmp)).from_buffer_copy(Message_tmp)

if wirehair.wirehair_init_(2) != Wirehair_Success :
    # this "2" can change in future wirehair releases. :-)
    # Just updated when necessary.
    print("Wirehair_Init() failed! exiting.")
    exit()

encoder = wirehair.wirehair_encoder_create(0, ctypes.byref(Message),
                                           ctypes.c_uint64(len(Message)),
                                           ctypes.c_uint32(KPacketSize.value))

if encoder == 0:
    print("Creation of encoder failed! exiting.")
    exit()

decoder = wirehair.wirehair_decoder_create(0,
                                           ctypes.c_uint64(len(Message)),
                                           ctypes.c_uint32(KPacketSize.value))

if decoder == 0:
    print("Creation of encoder failed! exiting.")
    wirehair.wirehair_free(encoder)
    exit()

blockid = ctypes.c_uint(0)
needed = ctypes.c_uint(0)

while True:

    blockid.value += 1

    # simulate 10% packet loss
    if (blockid.value % 10) == 0:
        continue

    needed.value += 1
    block = (ctypes.c_uint8 * KPacketSize.value)()
    # ? They are real need to redefining it always in loop ??
    # ? Will speedup define it before the while loop, just one time?

    # Encode a packet
    writelen = ctypes.c_uint32(0)
    encodedResult = wirehair.wirehair_encode(encoder, #encoder object
                                            ctypes.c_uint(blockid.value), #ID of block to generate
                                            ctypes.byref(block), #output buffer
                                            ctypes.c_uint32(KPacketSize.value), #output buffer size
                                            ctypes.byref(writelen)) # returned block length

    if encodedResult != Wirehair_Success:
        print("Wirehair_encode failed! exiting.")
        exit()

    decodeResult = wirehair.wirehair_decode(
        decoder,  # Decoder Object
        ctypes.c_uint(blockid.value),  # ID of block that was encoded
        ctypes.byref(block),  # input buffer
        writelen  # Block length
    )

    if decodeResult == Wirehair_Success:
        # Decoder has enough data to recover now
        break

    if decodeResult != Wirehair_NeedMore:
        print("Wirehair_decode failed: ", decodeResult, " \n")

decoded = (ctypes.c_uint8 * len(Message))()

# recover original data on decoder side
decodeResult = wirehair.wirehair_recover(
    decoder,
    ctypes.byref(decoded),
    ctypes.c_uint64(len(Message))
)

if decodeResult != 0:
    print("Wirehair_recover failed! exiting.")
    exit()

if decoded[:] == Message[:]:
    print("msgs are equal")

# just more for fun
eita = (ctypes.c_byte * len(Message)).from_buffer_copy(bytearray(decoded[:]))
print(str(eita, "ascii"))

wirehair.wirehair_free(encoder) ## ? are need to "free" from python? maybe not. :-)
wirehair.wirehair_free(decoder) ## fixme if necessary: if "wirehair.wirehair_free()" are causing trouble, remove they.

#  Obs.: tested in Linux Ubuntu Disco Jingo, gcc-8.3 and python 3.7.2+
#  in 19 march 2019

# Enjoy!! :-)
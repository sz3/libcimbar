/*
    Copyright (c) 2012-2018 Christopher A. Taylor.  All rights reserved.

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

#include "wirehair.h"
#include "WirehairCodec.h"

#include <new> // std::nothrow

static bool m_init = false;


extern "C" {


//-----------------------------------------------------------------------------
// Wirehair API

WIREHAIR_EXPORT const char *wirehair_result_string(
    WirehairResult result ///< Result code to convert to string
)
{
    static_assert(WirehairResult_Count == 11, "Update this switch too");

    switch (result)
    {
    case Wirehair_Success:           return "Wirehair_Success";
    case Wirehair_NeedMore:    return "Wirehair_NeedMore";
    case Wirehair_BadDenseSeed:      return "Wirehair_BadDenseSeed";
    case Wirehair_BadPeelSeed:       return "Wirehair_BadPeelSeed";
    case Wirehair_BadInput_SmallN:   return "Wirehair_BadInput_SmallN";
    case Wirehair_BadInput_LargeN:   return "Wirehair_BadInput_LargeN";
    case Wirehair_ExtraInsufficient: return "Wirehair_ExtraInsufficient";
    case Wirehair_InvalidInput:      return "Wirehair_InvalidInput";
    case Wirehair_OOM:               return "Wirehair_OOM";
    case Wirehair_UnsupportedPlatform: return "Wirehair_UnsupportedPlatform";
    default:
        break;
    }

    return "Unknown";
}

WIREHAIR_EXPORT WirehairResult wirehair_init_(int expected_version)
{
    // If version does not match:
    if (expected_version != WIREHAIR_VERSION) {
        return Wirehair_InvalidInput;
    }

    const int gfInitResult = gf256_init();

    // If gf256 init failed:
    if (gfInitResult != 0) {
        return Wirehair_UnsupportedPlatform;
    }

    m_init = true;
    return Wirehair_Success;
}

WIREHAIR_EXPORT WirehairCodec wirehair_encoder_create(
    WirehairCodec reuseOpt, ///< [Optional] Pointer to prior codec object
    const void*    message, ///< Pointer to message
    uint64_t  messageBytes, ///< Bytes in the message
    uint32_t    blockBytes  ///< Bytes in an output block
)
{
    // If input is invalid:
    if (!m_init || !message || messageBytes < 1 || blockBytes < 1) {
        return nullptr;
    }

    wirehair::Codec* codec = reinterpret_cast<wirehair::Codec*>(reuseOpt);

    // Allocate a new Codec object
    if (!codec) {
        codec = new (std::nothrow) wirehair::Codec;
    }

    // Initialize codec
    WirehairResult result = codec->InitializeEncoder(messageBytes, blockBytes);

    // If initialization succeeded:
    if (result == Wirehair_Success) {
        // Feed message to codec
        result = codec->EncodeFeed(message);
    }

    // If either function failed:
    if (result != Wirehair_Success)
    {
        // Note this will also release the reuse parameter
        delete codec;
        codec = nullptr;
    }

    return reinterpret_cast<WirehairCodec>(codec);
}

WIREHAIR_EXPORT WirehairResult wirehair_encode(
    WirehairCodec    codec, ///< Pointer to codec from wirehair_encoder_init()
    unsigned       blockId, ///< Identifier of block to generate
    void*     blockDataOut, ///< Pointer to output block data
    uint32_t      outBytes, ///< Bytes in the output buffer
    uint32_t* dataBytesOut  ///< Number of bytes written <= blockBytes
)
{
    if (!codec || !blockDataOut || !dataBytesOut) {
        return Wirehair_InvalidInput;
    }

    wirehair::Codec* session = reinterpret_cast<wirehair::Codec*>(codec);

    const uint32_t writtenBytes = session->Encode(blockId, blockDataOut, outBytes);
    *dataBytesOut = writtenBytes;

    if (writtenBytes <= 0) {
        return Wirehair_InvalidInput;
    }

    return Wirehair_Success;
}

WIREHAIR_EXPORT WirehairCodec wirehair_decoder_create(
    WirehairCodec reuseOpt, ///< Codec object to reuse
    uint64_t  messageBytes, ///< Bytes in the message to decode
    uint32_t    blockBytes  ///< Bytes in each encoded block
)
{
    // If input is invalid:
    if (messageBytes < 1 || blockBytes < 1) {
        return nullptr;
    }

    wirehair::Codec* codec = reinterpret_cast<wirehair::Codec*>(reuseOpt);

    // Allocate a new Codec object
    if (!codec) {
        codec = new (std::nothrow) wirehair::Codec;
    }

    // Allocate memory for decoding
    WirehairResult result = codec->InitializeDecoder(messageBytes, blockBytes);

    // If either function failed:
    if (result != Wirehair_Success)
    {
        // Note this will also release the reuse parameter
        delete codec;
        codec = nullptr;
    }

    return reinterpret_cast<WirehairCodec>(codec);
}

WIREHAIR_EXPORT WirehairResult wirehair_decode(
    WirehairCodec   codec, ///< Codec object
    unsigned      blockId, ///< ID number of received block
    const void* blockData, ///< Pointer to block data
    uint32_t    dataBytes  ///< Number of bytes in the data block
)
{
    // If input is invalid:
    if (!codec || !blockData || dataBytes < 1) {
        return Wirehair_InvalidInput;
    }

    wirehair::Codec* decoder = reinterpret_cast<wirehair::Codec*>(codec);

    return decoder->DecodeFeed(blockId, blockData, dataBytes);
}

WIREHAIR_EXPORT WirehairResult wirehair_recover(
    WirehairCodec    codec, ///< Codec object
    void*       messageOut, ///< Buffer where reconstructed message will be written
    uint64_t  messageBytes  ///< Bytes in the message
)
{
    // If input is invalid:
    if (!codec || !messageOut) {
        return Wirehair_InvalidInput;
    }

    wirehair::Codec* decoder = reinterpret_cast<wirehair::Codec*>(codec);

    return decoder->ReconstructOutput(messageOut, messageBytes);
}

WIREHAIR_EXPORT WirehairResult wirehair_recover_block(
    WirehairCodec codec, ///< Codec object
    unsigned    blockId, ///< ID of the block to reconstruct between 0..N-1
    void*  blockDataOut, ///< Pointer to block data
    uint32_t*  bytesOut  ///< Set to the number of data bytes in the block
)
{
    // If input is invalid:
    if (!codec || !blockDataOut || !bytesOut) {
        return Wirehair_InvalidInput;
    }

    wirehair::Codec* decoder = reinterpret_cast<wirehair::Codec*>(codec);

    return decoder->ReconstructBlock((uint16_t)blockId, blockDataOut, bytesOut);
}

WIREHAIR_EXPORT WirehairResult wirehair_decoder_becomes_encoder(
    WirehairCodec codec ///< Codec to change
)
{
    // If input is invalid:
    if (!codec) {
        return Wirehair_InvalidInput;
    }

    wirehair::Codec* encoder = reinterpret_cast<wirehair::Codec*>(codec);

    return encoder->InitializeEncoderFromDecoder();
}

WIREHAIR_EXPORT void wirehair_free(
    WirehairCodec codec ///< Codec object to free
)
{
    wirehair::Codec* object = reinterpret_cast<wirehair::Codec*>(codec);

    delete object;
}


} // extern "C"

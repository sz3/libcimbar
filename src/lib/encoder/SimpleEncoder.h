/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitreader.h"
#include "cimb_translator/CimbWriter.h"
#include "cimb_translator/Config.h"
#include "compression/zstd_compressor.h"
#include "fountain/fountain_encoder_stream.h"

#include <opencv2/opencv.hpp>
#include <optional>
#include <string>

class SimpleEncoder
{
public:
	SimpleEncoder(unsigned ecc_bytes=40, unsigned bits_per_symbol=0, unsigned bits_per_color=0);

	template <typename STREAM>
	std::optional<cv::Mat> encode_next(STREAM& stream);

	template <typename STREAM>
	fountain_encoder_stream::ptr create_fountain_encoder(STREAM& stream, int compression_level=6);


protected:
	unsigned _eccBytes;
	unsigned _bitsPerSymbol;
	unsigned _bitsPerColor;
};

inline SimpleEncoder::SimpleEncoder(unsigned ecc_bytes, unsigned bits_per_symbol, unsigned bits_per_color)
    : _eccBytes(ecc_bytes)
    , _bitsPerSymbol(bits_per_symbol? bits_per_symbol : cimbar::Config::symbol_bits())
    , _bitsPerColor(bits_per_color? bits_per_color : cimbar::Config::color_bits())
{
}

/* while bits == f.read(buffer, 8192)
 *     encode(bits)
 *
 * char buffer[2000];
 * while f.read(buffer)
 *     bit_buffer bb(buffer)
 *     while bb
 *         bits1 = bb.get(_bitsPerSymbol)
 *         bits2 = bb.get(_bitsPerColor)
 *
 * */

template <typename STREAM>
inline std::optional<cv::Mat> SimpleEncoder::encode_next(STREAM& stream)
{
	if (!stream.good())
		return std::nullopt;

	unsigned bits_per_op = _bitsPerColor + _bitsPerSymbol;
	CimbWriter writer;

	reed_solomon_stream rss(stream, _eccBytes);
	bitreader br;
	while (rss.good())
	{
		unsigned bytes = rss.readsome();
		if (bytes == 0)
			break;
		br.assign_new_buffer(rss.buffer(), bytes);
		while (!br.empty())
		{
			unsigned bits = br.read(bits_per_op);
			if (!br.partial())
				writer.write(bits);
		}
		if (writer.done())
			return writer.image();
	}
	// we don't have a full frame, but return what we've got
	return writer.image();
}

template <typename STREAM>
inline fountain_encoder_stream::ptr SimpleEncoder::create_fountain_encoder(STREAM& stream, int compression_level)
{
	unsigned chunk_size = cimbar::Config::fountain_chunk_size(_eccBytes);

	std::stringstream ss;
	if (compression_level <= 0)
		ss << stream.rdbuf();
	else
	{
		cimbar::zstd_compressor<std::stringstream> f;
		if (!f.compress(stream))
			return nullptr;

		// find size of compressed zstd stream, and pad it if necessary.
		size_t compressedSize = f.size();
		if (compressedSize < chunk_size)
			f.pad(chunk_size - compressedSize + 1);
		ss = std::move(f);
	}

	return fountain_encoder_stream::create(ss, chunk_size, 0); // will eventually do something clever with encode_id?
}


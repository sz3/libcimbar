/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitreader.h"
#include "cimb_translator/CimbWriter.h"
#include "cimb_translator/Config.h"
#include "compression/zstd_compressor.h"
#include "fountain/fountain_encoder_stream.h"
#include "serialize/format.h"
#include "serialize/str.h"

#include <opencv2/opencv.hpp>
#include <functional>
#include <optional>
#include <string>

class Encoder
{
public:
	Encoder(unsigned ecc_bytes=40, unsigned bits_per_symbol=0, unsigned bits_per_color=0);

	template <typename STREAM>
	std::optional<cv::Mat> encode_next(STREAM& stream);

	unsigned encode(const std::string& filename, std::string output_prefix);

	template <typename STREAM>
	fountain_encoder_stream::ptr create_fountain_encoder(STREAM& stream, int compression_level=6);

	unsigned encode_fountain(const std::string& filename, std::string output_prefix, int compression_level=6);
	unsigned encode_fountain(const std::string& filename, const std::function<bool(const cv::Mat&, unsigned)>& on_frame, int compression_level=6);

protected:
	unsigned _eccBytes;
	unsigned _bitsPerSymbol;
	unsigned _bitsPerColor;
};

inline Encoder::Encoder(unsigned ecc_bytes, unsigned bits_per_symbol, unsigned bits_per_color)
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
inline std::optional<cv::Mat> Encoder::encode_next(STREAM& stream)
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

inline unsigned Encoder::encode(const std::string& filename, std::string output_prefix)
{
	std::ifstream f(filename);

	unsigned i = 0;
	while (true)
	{
		auto frame = encode_next(f);
		if (!frame)
			break;

		std::string output = fmt::format("{}_{}.png", output_prefix, i);
		cv::imwrite(output, *frame);
		++i;
	}
	return i;
}

template <typename STREAM>
inline fountain_encoder_stream::ptr Encoder::create_fountain_encoder(STREAM& stream, int compression_level)
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

inline unsigned Encoder::encode_fountain(const std::string& filename, const std::function<bool(const cv::Mat&, unsigned)>& on_frame, int compression_level)
{
	std::ifstream infile(filename);
	fountain_encoder_stream::ptr fes = create_fountain_encoder(infile, compression_level);
	if (!fes)
		return 0;

	// With ecc = 40, we have 60 rs blocks * 115 bytes per block == 6900 bytes to work with.
	// the fountain_chunk_size will be 690.
	// fountain_chunks_per_frame() is currently a constant (10).
	unsigned requiredFrames = fes->blocks_required() * 2 / cimbar::Config::fountain_chunks_per_frame();
	if (requiredFrames == 0)
		requiredFrames = 1; // could also do +1 on the division above?

	unsigned i = 0;
	while (i < requiredFrames)
	{
		auto frame = encode_next(*fes);
		if (!frame)
			break;

		if (!on_frame(*frame, i))
			break;
		++i;
	}
	return i;
}

inline unsigned Encoder::encode_fountain(const std::string& filename, std::string output_prefix, int compression_level)
{
	std::function<bool(const cv::Mat&, unsigned)> fun = [output_prefix] (const cv::Mat& frame, unsigned i) {
		std::string output = fmt::format("{}_{}.png", output_prefix, i);
		return cv::imwrite(output, frame);
	};
	return encode_fountain(filename, fun, compression_level);
}

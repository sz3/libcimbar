#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitreader.h"
#include "cimb_translator/CimbWriter.h"
#include "cimb_translator/Config.h"
#include "fountain/fountain_encoder_stream.h"
#include "serialize/format.h"
#include "serialize/str.h"

#include <opencv2/opencv.hpp>
#include <optional>
#include <string>

class Encoder
{
public:
	Encoder(unsigned ecc_bytes=40, unsigned bits_per_symbol=0, unsigned bits_per_color=0);

	template <typename STREAM>
	std::optional<cv::Mat> encode_next(STREAM& stream);

	unsigned encode(const std::string& filename, std::string output_prefix);
	unsigned encode_fountain(const std::string& filename, std::string output_prefix);

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

		std::string output = fmt::format("{}-{}.png", output_prefix, i);
		cv::imwrite(output, *frame);
		++i;
	}
	return i;
}

inline unsigned Encoder::encode_fountain(const std::string& filename, std::string output_prefix)
{
	std::ifstream f(filename);
	fountain_encoder_stream fes = fountain_encoder_stream<626>::create(f);
	// 626 * 11 == 6886. With ecc = 40, we have 60 rs blocks * 115 bytes per block == 6900 bytes to work with. 14 are the header.
	// it would be nice to make this saner
	unsigned target = fes.blocks_required() * 2 / 11;

	std::vector<std::string> splits = turbo::str::split(filename, '/');
	std::string shortname = splits.size()? splits.back() : filename;

	unsigned i = 0;
	while (i < target)
	{
		// need to encode header at start of each frame...
		fes.encode_metadata_block(shortname);

		auto frame = encode_next(fes);
		if (!frame)
			break;

		std::string output = fmt::format("{}-{}.png", output_prefix, i);
		cv::imwrite(output, *frame);
		++i;
	}
	return i;
}


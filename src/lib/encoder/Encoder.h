#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitreader.h"
#include "cimb_translator/CimbWriter.h"
#include "cimb_translator/Config.h"
#include "serialize/format.h"

#include <opencv2/opencv.hpp>
#include <optional>
#include <string>

class Encoder
{
public:
	Encoder(unsigned bits_per_symbol=0, unsigned bits_per_color=0, unsigned ecc_bytes=15);

	template <typename BSTREAM>
	std::optional<cv::Mat> encode_next(BSTREAM& stream);

	unsigned encode(std::string filename, std::string output_prefix);

protected:
	unsigned _bitsPerSymbol;
	unsigned _bitsPerColor;
	unsigned _eccBytes;
};

inline Encoder::Encoder(unsigned bits_per_symbol, unsigned bits_per_color, unsigned ecc_bytes)
    : _bitsPerSymbol(bits_per_symbol? bits_per_symbol : cimbar::Config::symbol_bits())
    , _bitsPerColor(bits_per_color? bits_per_color : cimbar::Config::color_bits())
    , _eccBytes(ecc_bytes)
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

template <typename BSTREAM>
inline std::optional<cv::Mat> Encoder::encode_next(BSTREAM& stream)
{
	if (!stream.good())
		return std::nullopt;

	unsigned bits_per_op = _bitsPerColor + _bitsPerSymbol;
	CimbWriter writer;

	bitreader br;
	while (stream.good())
	{
		unsigned bytes = stream.readsome();
		if (bytes == 0)
			break;
		br.assign_new_buffer(stream.buffer(), bytes);
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

inline unsigned Encoder::encode(std::string filename, std::string output_prefix)
{
	std::ifstream f(filename);
	reed_solomon_stream rss(f, _eccBytes);

	int i = 0;
	while (true)
	{
		auto frame = encode_next(rss);
		if (!frame)
			break;

		std::string output = fmt::format("{}-{}.png", output_prefix, i);
		cv::imwrite(output, *frame);
		++i;
	}
	return i;
}

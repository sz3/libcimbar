/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitreader.h"
#include "bit_file/bitbuffer.h"
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
	SimpleEncoder(int ecc_bytes=-1, unsigned bits_per_symbol=0, int bits_per_color=-1);
	void set_legacy_mode();
	void set_encode_id(uint8_t encode_id); // [0-127] -- the high bit is ignored.

	template <typename STREAM>
	std::optional<cv::Mat> encode_next(STREAM& stream, int canvas_size=0);

	template <typename STREAM>
	fountain_encoder_stream::ptr create_fountain_encoder(STREAM& stream, int compression_level=6);

protected:
	template <typename STREAM>
	std::optional<cv::Mat> encode_next_coupled(STREAM& stream, int canvas_size=0);

protected:
	unsigned _eccBytes;
	unsigned _eccBlockSize;
	unsigned _bitsPerSymbol;
	unsigned _bitsPerColor;
	bool _dark;
	bool _coupled;
	unsigned _colorMode;
	uint8_t _encodeId = 0;
};

inline SimpleEncoder::SimpleEncoder(int ecc_bytes, unsigned bits_per_symbol, int bits_per_color)
	: _eccBytes(ecc_bytes >= 0? ecc_bytes : cimbar::Config::ecc_bytes())
	, _eccBlockSize(cimbar::Config::ecc_block_size())
	, _bitsPerSymbol(bits_per_symbol? bits_per_symbol : cimbar::Config::symbol_bits())
	, _bitsPerColor(bits_per_color >= 0? bits_per_color : cimbar::Config::color_bits())
	, _dark(cimbar::Config::dark())
	, _coupled(false)
	, _colorMode(cimbar::Config::color_mode())
{
}

inline void SimpleEncoder::set_legacy_mode()
{
	_coupled = true;
	_colorMode = 0;
}

inline void SimpleEncoder::set_encode_id(uint8_t encode_id)
{
	_encodeId = encode_id;
}

template <typename STREAM>
inline std::optional<cv::Mat> SimpleEncoder::encode_next(STREAM& stream, int canvas_size)
{
	if (_coupled)
		return encode_next_coupled(stream, canvas_size);

	if (!stream.good())
		return std::nullopt;

	unsigned bits_per_op = _bitsPerColor + _bitsPerSymbol;
	CimbWriter writer(_bitsPerSymbol, _bitsPerColor, _dark, _colorMode, canvas_size);

	unsigned numCells = writer.num_cells();
	bitbuffer bb(cimbar::Config::capacity(bits_per_op));

	unsigned bitPos = 0;
	unsigned endBitPos = numCells*bits_per_op;

	int progress = 0;
	unsigned bitsPerRead = _bitsPerSymbol;
	unsigned bitsPerWrite = bits_per_op;

	reed_solomon_stream rss(stream, _eccBytes, _eccBlockSize);
	bitreader br;
	while (rss.good() and progress < 2)  // 1 symbol pass + 1 color pass
	{
		unsigned bytes = rss.readsome();
		if (bytes == 0)
			break;
		br.assign_new_buffer(rss.buffer(), bytes);

		// reorder. We're encoding the symbol bits and striping them across the whole image
		// then encoding the color bits and striping them in the same way (filling in the gaps)
		while (!br.empty())
		{
			unsigned bits = br.read(bitsPerRead);
			if (!br.partial())
				bb.write(bits, bitPos, bitsPerWrite);
			bitPos += bits_per_op;

			if (bitPos >= endBitPos)
			{
				// switch to color section
				bitsPerRead = _bitsPerColor;
				bitsPerWrite = _bitsPerColor;
				bitPos = 0;
				++progress;
				break;
			}
		}
	}

	// dump whatever we have to image
	for (bitPos = 0; bitPos < endBitPos; bitPos+=bits_per_op)
	{
		unsigned bits = bb.read(bitPos, bits_per_op);
		writer.write(bits);
	}

	// return what we've got
	return writer.image();
}

template <typename STREAM>
inline std::optional<cv::Mat> SimpleEncoder::encode_next_coupled(STREAM& stream, int canvas_size)
{
	// the old way. Symbol and color bits are mixed together, limiting the color correction possibilities
	// but potentially allowing a lack of errors in one channel to correct errors in the other.
	// ... also, potentially allowing a preponderance of errors in one channel to doom the whole decode.
	// the net result is that best case performance *can* be better this way, but average and worst case
	// will be worse.
	if (!stream.good())
		return std::nullopt;

	unsigned bits_per_op = _bitsPerColor + _bitsPerSymbol;
	CimbWriter writer(_bitsPerSymbol, _bitsPerColor, _dark, _colorMode, canvas_size);

	reed_solomon_stream rss(stream, _eccBytes, _eccBlockSize);
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
	unsigned chunk_size = cimbar::Config::fountain_chunk_size(_eccBytes, _bitsPerColor + _bitsPerSymbol, (_colorMode==0 and _coupled));

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

	return fountain_encoder_stream::create(ss, chunk_size, _encodeId);
}


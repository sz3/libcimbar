/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitbuffer.h"
#include "cimb_translator/CimbDecoder.h"
#include "cimb_translator/CimbReader.h"
#include "cimb_translator/Config.h"
#include "cimb_translator/Interleave.h"

#include <opencv2/opencv.hpp>
#include <string>

class Decoder
{
public:
	Decoder(int ecc_bytes=-1, int color_bits=-1, bool interleave=true);

	template <typename MAT, typename STREAM>
	unsigned decode(const MAT& img, STREAM& ostream, bool should_preprocess=false, bool color_correction=true);

	template <typename MAT, typename STREAM>
	unsigned decode_fountain(const MAT& img, STREAM& ostream, bool should_preprocess=false, bool color_correction=true);

	unsigned decode(std::string filename, std::string output);

protected:
	template <typename STREAM>
	unsigned do_decode(CimbReader& reader, STREAM& ostream);

protected:
	unsigned _eccBytes;
	unsigned _eccBlockSize;
	unsigned _colorBits;
	unsigned _bitsPerOp;
	unsigned _interleaveBlocks;
	unsigned _interleavePartitions;
	CimbDecoder _decoder;
};

inline Decoder::Decoder(int ecc_bytes, int color_bits, bool interleave)
	: _eccBytes(ecc_bytes >= 0? ecc_bytes : cimbar::Config::ecc_bytes())
	, _eccBlockSize(cimbar::Config::ecc_block_size())
	, _colorBits(color_bits >= 0? color_bits : cimbar::Config::color_bits())
	, _bitsPerOp(cimbar::Config::symbol_bits() + _colorBits)
	, _interleaveBlocks(interleave? cimbar::Config::interleave_blocks() : 0)
	, _interleavePartitions(cimbar::Config::interleave_partitions())
	, _decoder(cimbar::Config::symbol_bits(), _colorBits, cimbar::Config::dark(), 0xFF)
{
}

/* while bits == f.read_tile()
 *     decode(bits)
 *
 * bitwriter bw("output.txt");
 * img = open("foo.png")
 * for tileX, tileY in img
 *     bits = decoder.decode(img, tileX, tileY)
 *     bw.write(bits)
 *     if bw.shouldFlush()
 *        bw.flush()
 *
 * */
template <typename STREAM>
inline unsigned Decoder::do_decode(CimbReader& reader, STREAM& ostream)
{
	bitbuffer bb(cimbar::Config::capacity(_bitsPerOp));
	std::vector<unsigned> interleaveLookup = Interleave::interleave_reverse(reader.num_reads(), _interleaveBlocks, _interleavePartitions);
	std::array<PositionData, 25920> colorPositions; // 25920 = the number of cells == reader.num_reads(). Can we calculate this from config at compile time?

	// read symbols first
	while (!reader.done())
	{
		// reader is in charge of the cell index (i) calculation
		// we can compute the bitindex ('index') here, but only the reader will know the right cell index...
		PositionData pos;
		unsigned bits = reader.read(pos);

		unsigned bitPos = interleaveLookup[pos.i] * _bitsPerOp;
		bb.write(bits, bitPos, _bitsPerOp);

		colorPositions[pos.i] = {bitPos, pos.x, pos.y};
	}

	// then decode colors.
	// the symbol+color decode could be done as one pass, but doing it as two gives us more control of the caches
	for (const PositionData& p : colorPositions)
	{
		unsigned bits = reader.read_color(p);
		bb.write(bits, p.i, _colorBits);
	}

	reed_solomon_stream rss(ostream, _eccBytes, _eccBlockSize);
	return bb.flush(rss);
}

// seems like we want to take a file or img, and have an output sink
// output sync could be template param?
// we'd decode the full message (via bit_writer) to a temp buffer -- probably a stringstream
// then we'd direct the stringstream to our sink
// which would either be a filestream, or a multi-channel fountain sink

template <typename MAT, typename STREAM>
inline unsigned Decoder::decode(const MAT& img, STREAM& ostream, bool should_preprocess, bool color_correction)
{
	CimbReader reader(img, _decoder, should_preprocess, color_correction);
	return do_decode(reader, ostream);
}

template <typename MAT, typename FOUNTAINSTREAM>
inline unsigned Decoder::decode_fountain(const MAT& img, FOUNTAINSTREAM& ostream, bool should_preprocess, bool color_correction)
{
	CimbReader reader(img, _decoder, should_preprocess, color_correction);

	aligned_stream aligner(ostream, ostream.chunk_size());
	return do_decode(reader, aligner);
}

inline unsigned Decoder::decode(std::string filename, std::string output)
{
	cv::Mat img = cv::imread(filename);
	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

	std::ofstream f(output);
	return decode(img, f, false);
}

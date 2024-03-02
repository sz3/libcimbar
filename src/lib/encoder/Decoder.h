/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitbuffer.h"
#include "cimb_translator/CimbDecoder.h"
#include "cimb_translator/CimbReader.h"
#include "cimb_translator/Config.h"
#include "cimb_translator/Interleave.h"
#include "util/File.h"
#include "util/null_stream.h"

#include <opencv2/opencv.hpp>
#include <functional>
#include <string>

class Decoder
{
public:
	Decoder(int ecc_bytes=-1, int color_bits=-1, bool interleave=true);

	template <typename MAT, typename STREAM>
	unsigned decode(const MAT& img, STREAM& ostream, unsigned color_mode=1, bool should_preprocess=false, int color_correction=2);

	template <typename MAT, typename STREAM>
	unsigned decode_fountain(const MAT& img, STREAM& ostream, unsigned color_mode=1, bool should_preprocess=false, int color_correction=2);

	unsigned decode(std::string filename, std::string output, unsigned color_mode=1);

	bool load_ccm(std::string filename);
	bool save_ccm(std::string filename);

protected:
	template <typename STREAM>
	unsigned do_decode(CimbReader& reader, STREAM& ostream, bool legacy_mode);

	template <typename STREAM>
	unsigned do_decode_coupled(CimbReader& reader, STREAM& ostream);

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
inline unsigned Decoder::do_decode(CimbReader& reader, STREAM& ostream, bool legacy_mode)
{
	if (legacy_mode)
		return do_decode_coupled(reader, ostream);

	std::vector<unsigned> interleaveLookup = Interleave::interleave_reverse(reader.num_reads(), _interleaveBlocks, _interleavePartitions);
	std::vector<PositionData> colorPositions;
	colorPositions.resize(reader.num_reads()); // the number of cells == reader.num_reads(). Can we calculate this from config at compile time? Do we care?

	unsigned bitsPerSymbol = cimbar::Config::symbol_bits();
	{
		bitbuffer symbolBits(cimbar::Config::capacity(bitsPerSymbol));
		// read symbols first
		while (!reader.done())
		{
			// reader is in charge of the cell index (i) calculation
			// we can compute the bitindex ('index') here, but only the reader will know the right cell index...
			PositionData pos;
			unsigned bits = reader.read(pos);

			unsigned bitPos = interleaveLookup[pos.i] * bitsPerSymbol; // bitspersymbol, *iff* we're in the new mode
			symbolBits.write(bits, bitPos, bitsPerSymbol);

			// TODO: simplify this function by not storing colorPositions?
			// this is how it was originally done (see `do_decode_coupled()`), but we should be able to calculate them on the fly now
			colorPositions[pos.i] = {interleaveLookup[pos.i] * _colorBits, pos.x, pos.y};
		}

		// flush symbols
		reed_solomon_stream rss(ostream, _eccBytes, _eccBlockSize);
		symbolBits.flush(rss);
	}

	// do color correction init, now that we (hopefully) have some fountain headers from the symbol decode
	reader.init_ccm(_colorBits, _interleaveBlocks, _interleavePartitions, cimbar::Config::fountain_chunks_per_frame(_bitsPerOp, legacy_mode));

	bitbuffer colorBits(cimbar::Config::capacity(_colorBits));
	// then decode colors.
	for (const PositionData& p : colorPositions)
	{
		unsigned bits = reader.read_color(p);
		colorBits.write(bits, p.i, _colorBits);
	}

	reed_solomon_stream rss(ostream, _eccBytes, _eccBlockSize);
	// flush() will return the (good) cumulative bytes written to the underlying stream
	return colorBits.flush(rss);
}

template <typename STREAM>
inline unsigned Decoder::do_decode_coupled(CimbReader& reader, STREAM& ostream)
{
	// the legacy decoder function. Symbol and color bits are grouped together (an individual cell is treated as ex:6 bits),
	// and the decode is done in two passes only for performance benefits (caching).
	bitbuffer bb(cimbar::Config::capacity(_bitsPerOp));
	std::vector<unsigned> interleaveLookup = Interleave::interleave_reverse(reader.num_reads(), _interleaveBlocks, _interleavePartitions);
	std::vector<PositionData> colorPositions;
	colorPositions.resize(reader.num_reads());

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
	// the symbol+color decode could be done as one pass, but doing it as two gives us better cache utilization
	for (const PositionData& p : colorPositions)
	{
		unsigned bits = reader.read_color(p);
		bb.write(bits, p.i, _colorBits);
	}

	reed_solomon_stream rss(ostream, _eccBytes, _eccBlockSize);
	return bb.flush(rss);
}

template <typename MAT, typename STREAM>
inline unsigned Decoder::decode(const MAT& img, STREAM& ostream, unsigned color_mode, bool should_preprocess, int color_correction)
{
	CimbReader reader(img, _decoder, color_mode, should_preprocess, color_correction);
	return do_decode(reader, ostream, color_mode==0);
}

template <typename MAT, typename FOUNTAINSTREAM>
inline unsigned Decoder::decode_fountain(const MAT& img, FOUNTAINSTREAM& ostream,  unsigned color_mode, bool should_preprocess, int color_correction)
{
	CimbReader reader(img, _decoder, color_mode, should_preprocess, color_correction);
	bool legacy_mode = color_mode == 0;
	unsigned chunk_size = cimbar::Config::fountain_chunk_size(_eccBytes, _bitsPerOp, legacy_mode);
	auto update_md_fun = std::bind(&CimbReader::update_metadata, &reader, std::placeholders::_1, std::placeholders::_2);

	// we don't want to feed the fountain stream bad data, so we eat the decode if we have a mismatch
	// we still might succeed the decode, in which case (hopefully) the positive bytes we return will
	// tell our caller to fix the underlying ostream so the next round will work.
	if (ostream.chunk_size() != chunk_size)
	{
		null_stream devnull;
		aligned_stream aligner(devnull, chunk_size, 0, update_md_fun);
		return do_decode(reader, aligner, legacy_mode);
	}

	aligned_stream aligner(ostream, ostream.chunk_size(), 0, update_md_fun);
	return do_decode(reader, aligner, legacy_mode);
}

inline unsigned Decoder::decode(std::string filename, std::string output, unsigned color_mode)
{
	cv::Mat img = cv::imread(filename);
	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

	std::ofstream f(output);
	return decode(img, f, color_mode, false);
}

inline bool Decoder::load_ccm(std::string filename)
{
	File f(filename);
	std::string data = f.read_all();
	if (data.size() < 3*3*4)
		return false;

	cv::Mat temp(3, 3, CV_32F, data.data());

	_decoder.update_color_correction(temp);
	return true;
}

inline bool Decoder::save_ccm(std::string filename)
{
	if (not _decoder.get_ccm().active())
		return false;

	cv::Mat temp(_decoder.get_ccm().mat());

	File f(filename, true);
	if (f.write(reinterpret_cast<const char*>(temp.data), temp.rows * temp.cols * temp.elemSize()) == 0)  // len will be 9*elemsize, but...
		return false;
	return true;
}


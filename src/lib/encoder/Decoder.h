/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitbuffer.h"
#include "cimb_translator/CimbDecoder.h"
#include "cimb_translator/CimbReader.h"
#include "cimb_translator/Config.h"
#include "cimb_translator/Interleave.h"
#include "util/null_stream.h"

#include <opencv2/opencv.hpp>
#include <functional>
#include <string>

class Decoder
{
public:
	Decoder(bool use_ecc=true, bool interleave=true);
	unsigned symbol_bits() const;

	template <typename MAT, typename STREAM>
	unsigned decode(const MAT& img, STREAM& ostream, bool should_preprocess=false, int color_correction=2);

	template <typename MAT, typename STREAM>
	unsigned decode_fountain(const MAT& img, STREAM& ostream, bool should_preprocess=false, int color_correction=2);

protected:
	template <typename STREAM>
	unsigned do_decode(CimbReader& reader, STREAM& ostream);

	template <typename STREAM>
	unsigned do_decode_coupled(CimbReader& reader, STREAM& ostream);

protected:
	bool _useEcc;
	bool _interleave;
	CimbDecoder _decoder;
};

inline Decoder::Decoder(bool use_ecc, bool interleave)
	: _useEcc(use_ecc)
	, _interleave(interleave)
	, _decoder(cimbar::Config::symbol_bits(), cimbar::Config::color_bits(), cimbar::Config::dark(), 0xFF)
{
}

inline unsigned Decoder::symbol_bits() const
{
	return _decoder.symbol_bits();
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
	if (cimbar::Config::legacy_mode())
		return do_decode_coupled(reader, ostream);

	unsigned eccBytes = _useEcc? cimbar::Config::ecc_bytes() : 0;
	unsigned eccBlockSize = cimbar::Config::ecc_block_size();
	unsigned colorBits = cimbar::Config::color_bits();
	unsigned interleaveBlocks = _interleave? cimbar::Config::interleave_blocks() : 0;
	unsigned interleavePartitions = cimbar::Config::interleave_partitions();

	unsigned bitsPerSymbol = cimbar::Config::symbol_bits();
	unsigned symCapacity = cimbar::Config::capacity(bitsPerSymbol);
	unsigned colorCapacity = cimbar::Config::capacity(colorBits);
	unsigned bitsPerOp = cimbar::Config::bits_per_cell();
	unsigned fountain_chunks_per_frame = cimbar::Config::fountain_chunks_per_frame(bitsPerOp);

	std::vector<unsigned> interleaveLookup = Interleave::interleave_reverse(reader.num_reads(), interleaveBlocks, interleavePartitions);
	std::vector<PositionData> colorPositions;
	colorPositions.resize(reader.num_reads()); // the number of cells == reader.num_reads(). Can we calculate this from config at compile time? Do we care?

	{
		bitbuffer symbolBuff(symCapacity);
		// read symbols first
		while (!reader.done())
		{
			// reader is in charge of the cell index (i) calculation
			// we can compute the bitindex ('index') here, but only the reader will know the right cell index...
			PositionData pos;
			unsigned bits = reader.read(pos);

			unsigned bitPos = interleaveLookup[pos.i] * bitsPerSymbol; // bitspersymbol, *iff* we're in the new mode
			symbolBuff.write(bits, bitPos, bitsPerSymbol);

			// TODO: simplify this function by not storing colorPositions?
			// this is how it was originally done (see `do_decode_coupled()`), but we should be able to calculate them on the fly now
			colorPositions[pos.i] = {interleaveLookup[pos.i] * colorBits, pos.x, pos.y};
		}

		// flush symbols
		reed_solomon_stream rss(ostream, eccBytes, eccBlockSize);
		symbolBuff.flush(rss);
	}

	// do color correction init, now that we (hopefully) have some fountain headers from the symbol decode
	reader.init_ccm(colorBits, interleaveBlocks, interleavePartitions, fountain_chunks_per_frame);

	bitbuffer colorBuff(colorCapacity);
	// then decode colors.
	for (const PositionData& p : colorPositions)
	{
		unsigned bits = reader.read_color(p);
		colorBuff.write(bits, p.i, colorBits);
	}

	reed_solomon_stream rss(ostream, eccBytes, eccBlockSize);
	// flush() will return the (good) cumulative bytes written to the underlying stream
	return colorBuff.flush(rss);
}

template <typename STREAM>
inline unsigned Decoder::do_decode_coupled(CimbReader& reader, STREAM& ostream)
{
	// the legacy decoder function. Symbol and color bits are grouped together (an individual cell is treated as ex:6 bits),
	// and the decode is done in two passes only for performance benefits (caching).
	unsigned eccBytes = _useEcc? cimbar::Config::ecc_bytes() : 0;
	unsigned eccBlockSize = cimbar::Config::ecc_block_size();
	unsigned colorBits = cimbar::Config::color_bits();
	unsigned bitsPerOp = cimbar::Config::bits_per_cell();
	unsigned interleaveBlocks = _interleave? cimbar::Config::interleave_blocks() : 0;
	unsigned interleavePartitions = cimbar::Config::interleave_partitions();

	bitbuffer bb(cimbar::Config::capacity(bitsPerOp));
	std::vector<unsigned> interleaveLookup = Interleave::interleave_reverse(reader.num_reads(), interleaveBlocks, interleavePartitions);
	std::vector<PositionData> colorPositions;
	colorPositions.resize(reader.num_reads());

	// read symbols first
	while (!reader.done())
	{
		// reader is in charge of the cell index (i) calculation
		// we can compute the bitindex ('index') here, but only the reader will know the right cell index...
		PositionData pos;
		unsigned bits = reader.read(pos);

		unsigned bitPos = interleaveLookup[pos.i] * bitsPerOp;
		bb.write(bits, bitPos, bitsPerOp);

		colorPositions[pos.i] = {bitPos, pos.x, pos.y};
	}

	// then decode colors.
	// the symbol+color decode could be done as one pass, but doing it as two gives us better cache utilization
	for (const PositionData& p : colorPositions)
	{
		unsigned bits = reader.read_color(p);
		bb.write(bits, p.i, colorBits);
	}

	reed_solomon_stream rss(ostream, eccBytes, eccBlockSize);
	return bb.flush(rss);
}

template <typename MAT, typename STREAM>
inline unsigned Decoder::decode(const MAT& img, STREAM& ostream, bool should_preprocess, int color_correction)
{
	CimbReader reader(img, _decoder, cimbar::Config::color_mode(), should_preprocess, color_correction);
	return do_decode(reader, ostream);
}

template <typename MAT, typename FOUNTAINSTREAM>
inline unsigned Decoder::decode_fountain(const MAT& img, FOUNTAINSTREAM& ostream, bool should_preprocess, int color_correction)
{
	CimbReader reader(img, _decoder, cimbar::Config::color_mode(), should_preprocess, color_correction);
	unsigned chunk_size = cimbar::Config::fountain_chunk_size();
	auto update_md_fun = std::bind(&CimbReader::update_metadata, &reader, std::placeholders::_1, std::placeholders::_2, chunk_size);

	// we don't want to feed the fountain stream bad data, so we eat the decode if we have a mismatch
	// we still might succeed the decode, in which case (hopefully) the positive bytes we return will
	// tell our caller to fix the underlying ostream so the next round will work.
	if (ostream.chunk_size() != chunk_size)
	{
		null_stream devnull;
		aligned_stream aligner(devnull, chunk_size, 0, update_md_fun);
		return do_decode(reader, aligner);
	}

	aligned_stream aligner(ostream, ostream.chunk_size(), 0, update_md_fun);
	return do_decode(reader, aligner);
}

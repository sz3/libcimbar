#pragma once

#include "reed_solomon_stream.h"
#include "bit_file/bitwriter.h"
#include "cimb_translator/CimbDecoder.h"
#include "cimb_translator/CimbReader.h"
#include "cimb_translator/Config.h"
#include "cimb_translator/Interleave.h"
#include <opencv2/opencv.hpp>
#include <string>

class Decoder
{
public:
	Decoder(unsigned ecc_bytes=15, unsigned bits_per_op=0, bool interleave=true);

	template <typename STREAM>
	unsigned decode(const cv::Mat& img, STREAM& ostream, bool should_preprocess=false);

	unsigned decode(std::string filename, std::string output);

protected:
	template <typename STREAM>
	unsigned do_decode(CimbReader& reader, STREAM& ostream);

protected:
	unsigned _eccBytes;
	unsigned _bitsPerOp;
	unsigned _interleaveBlocks;
	CimbDecoder _decoder;
};

inline Decoder::Decoder(unsigned ecc_bytes, unsigned bits_per_op, bool interleave)
    : _eccBytes(ecc_bytes)
    , _bitsPerOp(bits_per_op? bits_per_op : cimbar::Config::bits_per_cell())
    , _interleaveBlocks(interleave? cimbar::Config::interleave_blocks() : 0)
    , _decoder(cimbar::Config::symbol_bits(), cimbar::Config::color_bits())
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
	// reed_solomon_stream::buffer_size*6 ... probably need partial flushes to make bits line up with rss...
	bitwriter<930> bw;
	std::stringstream buff;
	reed_solomon_stream rss(buff, _eccBytes);

	unsigned bytesWritten = 0;
	for (unsigned i : Interleave::interleave_indices(reader.num_reads(), _interleaveBlocks))
	{
		if (reader.done())
			break;
		unsigned bits = reader.read();
		bw.write(bits, _bitsPerOp);
		if (bw.shouldFlush())
			bytesWritten = bw.flush(rss);
	}

	// flush once more
	bytesWritten = bw.flush(rss);

	// make the buffer we pass to ostream contiguous
	ostream << buff.str();
	return bytesWritten;
}

// seems like we want to take a file or img, and have an output sink
// output sync could be template param?
// we'd decode the full message (via bit_writer) to a temp buffer -- probably a stringstream
// then we'd direct the stringstream to our sink
// which would either be a filestream, or a multi-channel fountain sink

template <typename STREAM>
inline unsigned Decoder::decode(const cv::Mat& img, STREAM& ostream, bool should_preprocess)
{
	CimbReader reader(img, _decoder, should_preprocess);
	return do_decode(reader, ostream);
}

inline unsigned Decoder::decode(std::string filename, std::string output)
{
	cv::Mat img = cv::imread(filename);
	std::ofstream f(output);
	return decode(img, f, false);
}

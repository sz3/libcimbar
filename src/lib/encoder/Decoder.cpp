#include "Decoder.h"

#include "reed_solomon_stream.h"
#include "bit_file/bitwriter.h"
#include "cimb_translator/CimbReader.h"
#include "cimb_translator/Config.h"

#include <fstream>
#include <string>
using std::string;
using namespace cimbar;


namespace {
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
	unsigned do_decode(CimbReader& reader, unsigned ecc_bytes, string output, unsigned bits_per_op)
	{
		bitwriter<> bw;
		std::ofstream f(output);
		reed_solomon_stream rss(f, ecc_bytes);

		unsigned bytesWritten = 0;
		while (!reader.done())
		{
			unsigned bits = reader.read();
			bw.write(bits, bits_per_op);
			if (bw.shouldFlush())
				bytesWritten = bw.flush(rss);
		}

		// flush once more
		bytesWritten = bw.flush(rss);
		return bytesWritten;
	}
}


Decoder::Decoder(unsigned ecc_bytes, unsigned bits_per_op)
    : _eccBytes(ecc_bytes)
    , _bitsPerOp(bits_per_op? bits_per_op : Config::bits_per_cell())
    , _decoder(Config::symbol_bits(), Config::color_bits())
{
}

unsigned Decoder::decode(const cv::Mat& img, std::string output)
{
	CimbReader reader(img, _decoder);
	return do_decode(reader, _eccBytes, output, _bitsPerOp);
}

unsigned Decoder::decode(string filename, string output)
{
	CimbReader reader(filename, _decoder);
	return do_decode(reader, _eccBytes, output, _bitsPerOp);
}

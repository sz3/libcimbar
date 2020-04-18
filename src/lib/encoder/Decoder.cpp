#include "Decoder.h"

#include "bit_file/bitwriter.h"
#include "cimb_translator/CimbReader.h"
#include "cimb_translator/Config.h"
#include "util/File.h"

#include <string>
using std::string;


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
	unsigned do_decode(CimbReader& reader, string output, unsigned bits_per_op)
	{
		bitwriter bw;
		File f(output, true);

		unsigned bytesWritten = 0;
		while (!reader.done())
		{
			unsigned bits = reader.read();
			bw.write(bits, bits_per_op);
			if (bw.shouldFlush())
				bytesWritten += bw.flush(f);
		}

		// flush once more
		bytesWritten += bw.flush(f);
		return bytesWritten;
	}
}


Decoder::Decoder(unsigned bits_per_op)
    : _bits_per_op(bits_per_op? bits_per_op : cimbar::Config::bits_per_cell())
{
}

unsigned Decoder::decode(const cv::Mat& img, std::string output)
{
	CimbReader reader(img);
	return do_decode(reader, output, _bits_per_op);
}

unsigned Decoder::decode(string filename, string output)
{
	CimbReader reader(filename);
	return do_decode(reader, output, _bits_per_op);
}

#include "Decoder.h"

#include "bit_file/bitwriter.h"
#include "cimb_translator/CimbReader.h"
#include "util/File.h"

#include <string>
using std::string;

Decoder::Decoder()
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


unsigned Decoder::decode(string filename, string output)
{
	unsigned bits_per_op = 6;
	CimbReader reader(filename);
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

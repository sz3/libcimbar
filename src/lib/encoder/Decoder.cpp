#include "Decoder.h"

#include "bit_file/bitreader.h"
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


unsigned Decoder::decode(string filename)
{
	unsigned bits_per_op = 6;

	File f(filename);

	char buffer[8192];
	while (f.good())
	{
		bitreader br(buffer, 8192);
		unsigned bits = br.read(bits_per_op);
	}
}

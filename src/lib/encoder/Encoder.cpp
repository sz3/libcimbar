#include "Encoder.h"

#include "bit_file/bitreader.h"
#include "util/File.h"

#include <string>
using std::string;

Encoder::Encoder(unsigned bits_per_symbol, unsigned bits_per_color)
    : _bitsPerSymbol(bits_per_symbol)
    , _bitsPerColor(bits_per_color)
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


unsigned Encoder::encode(string filename, string output)
{
	unsigned bits_per_op = _bitsPerColor + _bitsPerSymbol;
	unsigned readSize = 8192;  // should be a multiple of bits_per_op and 8

	char buffer[readSize];
	File f(filename);
	while (f.good())
	{
		unsigned bytes = f.read(buffer, readSize);
		bitreader br(buffer, bytes);
		while (!br.empty())
		{
			unsigned bits = br.read(bits_per_op);
			std::cout << "read " << bits << std::endl;
		}
	}
}

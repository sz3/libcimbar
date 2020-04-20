#include "Encoder.h"

#include "bit_file/bitreader.h"
#include "cimb_translator/ICimbWriter.h"
#include "util/File.h"

#include <string>
using std::string;


static const unsigned bufferSize = 155;
static const unsigned eccBytes = 10;


Encoder::Encoder(ICimbWriter& writer, unsigned bits_per_symbol, unsigned bits_per_color, unsigned ecc_bytes)
    : _writer(writer)
    , _bitsPerSymbol(bits_per_symbol)
    , _bitsPerColor(bits_per_color)
    , _eccBytes(ecc_bytes) // this is used as a boolean flag until we adjust the reed solomon code to pass ecc bytes at runtime
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
	unsigned readSize = bufferSize - eccBytes;

	//RS::ReedSolomon<bufferSize, 10> rs;
	bitreader br;

	char buffer[bufferSize] = {0};
	char landing[bufferSize];
	File f(filename);
	while (f.good())
	{
		unsigned bytes = f.read(buffer, readSize);
		//rs.Encode(buffer, landing);

		br.assign_new_buffer(buffer, bytes);
		while (!br.empty())
		{
			unsigned bits = br.read(bits_per_op);
			if (!br.partial())
				_writer.write(bits);
		}
	}
	_writer.save(output);
}

#include "Encoder.h"

#include "ReedSolomonFile.h"
#include "bit_file/bitreader.h"
#include "cimb_translator/ICimbWriter.h"

#include <string>
using std::string;


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

	bitreader br;
	ReedSolomonFile f(filename, _eccBytes);
	while (f.good())
	{
		unsigned bytes = f.read();
		br.assign_new_buffer(f.buffer(), bytes);
		while (!br.empty())
		{
			unsigned bits = br.read(bits_per_op);
			if (!br.partial())
				_writer.write(bits);
		}
	}
	_writer.save(output);
	return 0;
}

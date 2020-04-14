#pragma once

#include <string>

class ICimbWriter;

class Encoder
{
public:
	Encoder(ICimbWriter& writer, unsigned bits_per_symbol, unsigned bits_per_color); // pass in handler interface

	unsigned encode(std::string filename, std::string output);

protected:
	ICimbWriter& _writer;
	unsigned _bitsPerSymbol;
	unsigned _bitsPerColor;
};

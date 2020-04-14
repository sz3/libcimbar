#pragma once

#include <string>

class Encoder
{
public:
	Encoder(unsigned bits_per_symbol, unsigned bits_per_color); // pass in handler interface

	unsigned encode(std::string filename, std::string output);

protected:
	unsigned _bitsPerSymbol;
	unsigned _bitsPerColor;
};

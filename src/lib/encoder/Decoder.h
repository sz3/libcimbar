#pragma once

#include <string>

class Decoder
{
public:
	Decoder(); // pass in handler interface

	unsigned decode(std::string filename);

protected:
};

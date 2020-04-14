#pragma once

class bitwriter
{
public:
	bitwriter(char* buffer, unsigned size)
	    : _buffer(buffer)
	    , _size(size)
	{
	}

	bool writeBit(bool bit)
	{

	}

protected:
	char* _buffer;
	unsigned _size;
};

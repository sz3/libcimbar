#include <iostream>
#include <tuple>

class bitreader
{
public:
	bitreader(const char* buffer, unsigned size)
	    : _buffer(buffer)
	    , _size(size)
	    , _position(0)
	    , _bitoffset(0)
	{
	}

	bool empty() const
	{
		return _position >= _size;
	}

	unsigned readBit()
	{
		if (empty())
			return 0;

		unsigned bit = (_buffer[_position] & (1 << (7 - _bitoffset)));
		++_bitoffset;
		if (_bitoffset == 8)
		{
			_position += 1;
			_bitoffset -= 8;
		}
		return bit? 1 : 0;
	}

	unsigned read(unsigned how_many_bits)
	{
		//auto [bytes, bits] = bytes_and_bits(how_many_bits, _bitoffset);

		unsigned res = 0;
		for (unsigned i = 0; i < how_many_bits; ++i)
		{
			unsigned bit = readBit();
			res |= bit << (how_many_bits - i - 1);
		}
		return res;
	}

	std::tuple<unsigned, unsigned> bytes_and_bits(unsigned how_many_bits, unsigned bitoffset) const
	{
		unsigned remainder_bits = how_many_bits % 8;
		unsigned bytes = how_many_bits / 8;
		return {bytes, remainder_bits};
	}

protected:
	const char* _buffer;
	unsigned _size;
	unsigned _position;
	unsigned _bitoffset;
};

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include <iostream>
#include <tuple>

class bitreader
{
public:
	bitreader(const char* buffer, unsigned size)
	{
		assign_new_buffer(buffer, size);
		clear_partial();
	}

	bitreader()
	    : bitreader(NULL, 0)
	{
	}

	void assign_new_buffer(const char* buffer, unsigned size)
	{
		_buffer = buffer;
		_size = size;
		_position = 0;
		_bitoffset = 0;
	}

	bool empty() const
	{
		return _position >= _size;
	}

	unsigned partial() const
	{
		return _partialSize;
	}

	void set_partial(unsigned bits, unsigned num_bits)
	{
		_partial = bits;
		_partialSize = num_bits;
	}

	void clear_partial()
	{
		_partial = 0;
		_partialSize = 0;
	}

	int readBit()
	{
		if (empty())
			return -1;

		unsigned bit = (_buffer[_position] & (1 << (7 - _bitoffset)));
		++_bitoffset;
		if (_bitoffset == 8)
		{
			_position += 1;
			_bitoffset -= 8;
		}
		return bit? 1 : 0;
	}

	unsigned read(unsigned how_many_bits, unsigned& bits)
	{
		//auto [bytes, bits] = bytes_and_bits(how_many_bits, _bitoffset);

		bits = _partial;
		unsigned i = _partialSize;
		if (_partialSize)
			clear_partial();
		for (; i < how_many_bits; ++i)
		{
			int bit = readBit();
			if (bit < 0)
				return i;
			bits |= static_cast<unsigned>(bit) << (how_many_bits - i - 1);
		}
		return how_many_bits;
	}

	unsigned read(unsigned how_many_bits)
	{
		unsigned bits;
		unsigned res = read(how_many_bits, bits);
		if (res != how_many_bits)
			set_partial(bits, res);
		return bits;
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

	unsigned _partial;
	unsigned _partialSize;
};

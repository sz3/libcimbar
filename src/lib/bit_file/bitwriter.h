#pragma once

#include "util/File.h"
#include <array>
#include <iostream>

// write bits -> buffer
template <unsigned _capacity=2520>
class bitwriter
{
public:
	bitwriter()
	{
		clear();
	}

	bool writeBit(bool bit)
	{
		bit = bit? 1 : 0;
		if (shouldFlush())
			return false;

		char next = bit << (7 - _currentBit);
		_buffer[_currentByte] = _buffer[_currentByte] | next;
		if (++_currentBit > 7)
		{
			_currentBit -= 8;
			++_currentByte;
		}
		return true;
	}

	bool write(unsigned bits, int length)
	{
		// write the `length` LSB of `bits` to buffer
		for (int i = length-1; i >= 0; --i)
		{
			bool b = bits & (1 << i);
			writeBit(b);
		}
		return true;
	}

	template <typename STREAM>
	long flush(STREAM& f)
	{
		f.write(buffer().data(), _currentByte);
		clear();
		return f.tellp();
	}

	bool shouldFlush() const
	{
		return _currentByte == _capacity;
	}

	void clear()
	{
		_buffer = {0};
		_currentBit = 0;
		_currentByte = 0;
	}

	const std::array<char, _capacity>& buffer() const
	{
		return _buffer;
	}

protected:
	std::array<char, _capacity> _buffer; // maybe don't use std::array?
	unsigned _currentBit;
	unsigned _currentByte;
};

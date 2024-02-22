/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <vector>

// write bits -> buffer
class bitbuffer
{
public:
	class writer
	{
	public:
		writer(bitbuffer& bb, size_t pos=0)
		    : _bb(bb)
		    , _pos(pos)
		{}

		template <typename UINT>
		bool write_byte(UINT byte)
		{
			return write(byte, 8);
		}

		template <typename UINT>
		bool write(UINT byte, int length)
		{
			size_t pos = _pos;
			_pos += length;
			return _bb.write(byte, pos, length);
		}

		writer& operator<<(uint8_t byte)
		{
			write_byte(byte);
			return *this;
		}

	protected:
		bitbuffer& _bb;
		size_t _pos;
	};

public:
	bitbuffer(unsigned size_hint=9300)
	    : _sizeHint(size_hint)
	{
		clear();
	}

	void resize(unsigned length)
	{
		size_t requestedSize = length/8;
		size_t size = _buffer.size();
		if (size >= requestedSize)
			return;

		while (size < requestedSize)
			size += _sizeHint;
		_buffer.resize(size, 0);
	}

	bool write(unsigned data, unsigned index, int length)
	{
		resize(index+length);

		int currentByte = index/8;
		int currentBit = index%8;

		int nextWrite = std::min(length, 8-currentBit); // write this many bits
		while (length > 0 and nextWrite > 0)
		{
			unsigned char bits = data >> (length - nextWrite);
			bits = bits << (8 - nextWrite - currentBit);
			_buffer[currentByte] |= bits;

			length -= nextWrite;
			currentBit += nextWrite;
			if (currentBit >= 8)
				currentBit = 0;
			currentByte += 1;
			nextWrite = std::min(length, 8-currentBit);
		}
		return true;
	}

	unsigned read(unsigned index, int length) const
	{
		int currentByte = index/8;
		int currentBit = index%8;

		unsigned res = 0;
		int nextRead = std::min(length, 8-currentBit); // read this many bits
		while (length > 0)
		{
			unsigned char bits = static_cast<unsigned char>(_buffer[currentByte]) << currentBit;
			bits = bits >> (8-nextRead);
			res |= bits << (length - nextRead);

			length -= nextRead;
			currentBit += nextRead;
			if (currentBit >= 8)
				currentBit = 0;
			currentByte += 1;
			nextRead = std::min(length, 8-currentBit);
		}
		return res;
	}

	template <typename STREAM>
	long flush(STREAM& f)
	{
		f.write(buffer().data(), buffer().size());
		clear();
		return f.tellp();
	}

	void clear()
	{
		_buffer = {0};
		_buffer.resize(_sizeHint, 0);
	}

	const std::vector<char>& buffer() const
	{
		return _buffer;
	}

	void copy_to_buffer(const char* data, unsigned size)
	{
		_buffer.resize(size, 0);
		std::copy(data, data+size, _buffer.data());
	}

	writer get_writer(size_t pos=0)
	{
		return writer(*this, pos);
	}

protected:
	std::vector<char> _buffer;
	unsigned _sizeHint;
};

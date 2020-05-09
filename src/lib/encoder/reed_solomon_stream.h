#pragma once

#include "ReedSolomon.h"
#include <vector>

template <typename STREAM>
class reed_solomon_stream
{
public:
	static const unsigned buffer_size = 155;

public:
	reed_solomon_stream(STREAM& stream, unsigned ecc)
		: _stream(stream)
		, _rs(ecc)
	{
		_buffer.resize(buffer_size, 0);
	}

	ssize_t readsome(char* data=NULL, unsigned length=buffer_size)
	{
		if (!data)
			data = _buffer.data();
		ssize_t bytes = _stream.readsome(data, length - _rs.parity());
		if (!_rs.parity() or bytes <= 0)
			return bytes;
		// else
		_rs.encode(data, bytes, data);
		return buffer_size;
	}

	reed_solomon_stream& write(const char* data, unsigned length)
	{
		if (!_rs.parity())
		{
			_stream.write(data, length);
			return *this;
		}

		// else
		ssize_t bytes = _rs.decode(data, length, _buffer.data());
		if (bytes <= 0)
			_stream << ReedSolomon::BadChunk(buffer_size);
		else
			_stream.write(_buffer.data(), bytes);
		return *this;
	}

	const char* buffer() const
	{
		return _buffer.data();
	}

protected:
	std::vector<char> _buffer;
	STREAM& _stream;
	ReedSolomon _rs;
};

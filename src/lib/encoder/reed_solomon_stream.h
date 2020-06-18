#pragma once

#include "ReedSolomon.h"
#include "encoder/aligned_stream.h"
#include <fstream>
#include <sstream>
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
	    , _good(stream.good())
	{
		_buffer.resize(buffer_size, 0);
	}

	bool good() const
	{
		return _good and _stream.good();
	}

	long tellp() const
	{
		return _stream.tellp();
	}

	std::streamsize readsome(char* data=NULL, unsigned length=buffer_size)
	{
		if (!data)
			data = _buffer.data();

		_stream.read(data, length - _rs.parity());
		std::streamsize bytes = _stream.gcount();
		if (bytes <= 0)
		{
			_good = false;
			return bytes;
		}
		if (!_rs.parity())
			return bytes;

		// else
		_rs.encode(data, bytes, data);
		return buffer_size;
	}

	reed_solomon_stream& write(const char* data, unsigned length)
	{
		// length should be a multiple of buffer_size
		// we might implement some "leftovers" functionality, e.g. storing how many bytes were unprocessed
		if (!_rs.parity())
		{
			_stream.write(data, length);
			return *this;
		}

		// else
		while (length >= buffer_size)
		{
			ssize_t bytes = _rs.decode(data, buffer_size, _buffer.data());
			if (bytes <= 0)
				_stream << ReedSolomon::BadChunk(buffer_size - _rs.parity());
			else
				_stream.write(_buffer.data(), bytes);

			length -= buffer_size;
			data += buffer_size;
		}
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
	bool _good;
};

inline std::ifstream& operator<<(std::ifstream& s, const ReedSolomon::BadChunk& chunk)
{
	return s;
}

inline std::ofstream& operator<<(std::ofstream& os, const ReedSolomon::BadChunk& chunk)
{
	os << std::string(chunk.size, '\0');
	return os;
}

inline std::stringstream& operator<<(std::stringstream& s, const ReedSolomon::BadChunk& chunk)
{
	std::string temp(chunk.size, '\0');
	s.write(temp.data(), temp.size());
	return s;
}

template <typename STREAM>
inline aligned_stream<STREAM>& operator<<(aligned_stream<STREAM>& s, const ReedSolomon::BadChunk& chunk)
{
	s.mark_bad_chunk(chunk.size);
	return s;
}

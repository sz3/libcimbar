#pragma once

#include "ReedSolomon.h"
#include "util/File.h"
#include <vector>

class ReedSolomonFile : public File
{
public:
	static const unsigned buffer_size = 155;

public:
	ReedSolomonFile(std::string filename, unsigned ecc, bool write=false)
		: File(filename, write)
		, _rs(ecc)
	{
		_buffer.resize(buffer_size, 0);
	}

	unsigned read(char* data=NULL, unsigned length=buffer_size)
	{
		if (!data)
			data = _buffer.data();
		unsigned bytes = File::read(data, length - _rs.parity());
		if (!_rs.parity())
			return bytes;
		// else
		_rs.encode(data, bytes, data);
		return buffer_size;
	}

	unsigned write(const char* data, unsigned length)
	{
		if (!_rs.parity())
			return File::write(data, length);

		// else
		ssize_t bytes = _rs.decode(data, length, _buffer.data());
		if (bytes <= 0)
			return 0;
		return File::write(_buffer.data(), bytes);
	}

	const char* buffer() const
	{
		return _buffer.data();
	}

protected:
	std::vector<char> _buffer;
	ReedSolomon _rs;
};

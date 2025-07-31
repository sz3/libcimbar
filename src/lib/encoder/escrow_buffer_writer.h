/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <algorithm>
#include <string>

class escrow_buffer_writer
{
public:
	escrow_buffer_writer(unsigned char* bufspace, unsigned bufcount, unsigned bufsize)
		: _bufspace(bufspace)
		, _bufcount(bufcount)
		, _bufsize(bufsize)
	{
	}

	bool good() const
	{
		return _good;
	}

	unsigned chunk_size() const
	{
		return _bufsize;
	}

	long tellp() const
	{
		return _totalCount;
	}

	unsigned buffers_in_use() const
	{
		return _bufIdx;
	}

	escrow_buffer_writer& write(const char* data, unsigned length)
	{
		// TODO: should this also act on multiples of `length`?

		// we can only write if the bufsize matches
		// and if we have buffers left
		if (length != _bufsize or _bufIdx>= _bufcount)
			_good = false;

		if (!good())
			return *this;

		std::copy(data, data+length, _bufspace+(_bufIdx*_bufsize));

		_totalCount += length;
		++_bufIdx;
		return *this;
	}

	escrow_buffer_writer& operator<<(const std::string& buffer)
	{
		return write(buffer.data(), buffer.size());
	}

protected:
	unsigned char* _bufspace;
	unsigned _bufcount;
	unsigned _bufsize;

	unsigned _bufIdx = 0;
	long _totalCount = 0;
	bool _good = true;
};

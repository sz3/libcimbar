/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "FountainDecoder.h"
#include <iostream>
#include <optional>
#include <string>

class fountain_decoder_stream
{
public:
	static const unsigned _headerSize = 6;

public:
	fountain_decoder_stream(unsigned data_size, unsigned buffer_size)
	    : _buffer(buffer_size, 0)
	    , _decoder(data_size, block_size())
	{
	}

	unsigned progress() const
	{
		return _decoder.progress();
	}

	unsigned blocks_required() const
	{
		return (data_size() / block_size()) + 1;
	}

	unsigned block_size() const
	{
		return _buffer.size() - _headerSize;
	}

	size_t data_size() const
	{
		return _decoder.length();
	}

	bool good() const
	{
		return _decoder.good();
	}

	std::optional<std::vector<uint8_t>> decode()
	{
		// if we're full
		_buffIndex = 0;
		// we ignore the first 4 bytes. It's the sink's job to make sure we're getting the right stuff.
		// we may, at some point, sanity check if data_size == [1]+[2]+[3]
		unsigned blockId = (unsigned)(_buffer[4]) << 8 | _buffer[5];
		return _decoder.decode(blockId, _buffer.data() + _headerSize, block_size());
	}

	// we need to track either:
	// 1. all packet header locations + current location in frame buffer to correlate
	// 2. current location in frame buffer to see if we're at a packet header location
	// 3. special case of #2, where we just roll forward every _bufferSize bytes?
	std::optional<std::vector<uint8_t>> write(const char* data, unsigned length)
	{
		while (length > 0 and good())
		{
			unsigned writeLen = std::min(length, (unsigned)(_buffer.size() - _buffIndex));
			uint8_t* dst = _buffer.data() + _buffIndex;
			std::copy(data, data + writeLen, dst);

			_buffIndex += writeLen;
			data += writeLen;
			length -= writeLen;

			if (_buffIndex == _buffer.size())
			{
				auto res = decode();
				if (res)
					return res;
			}
		}
		return std::nullopt;
	}

protected:
	std::vector<uint8_t> _buffer;
	FountainDecoder _decoder;
	unsigned _buffIndex = 0;
};

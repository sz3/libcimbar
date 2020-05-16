#pragma once

#include "FountainDecoder.h"
#include <iostream>
#include <optional>
#include <string>

template <unsigned _bufferSize=830>
class fountain_decoder_stream
{
public:
	static const unsigned _headerSize = 3;

public:
	fountain_decoder_stream(unsigned dataSize)
		: _decoder(dataSize, block_size())
	{
	}

	unsigned block_size() const
	{
		return _bufferSize - _headerSize;
	}

	bool good() const
	{
		return _decoder.good();
	}

	std::optional<std::vector<uint8_t>> decode()
	{
		// if we're full
		_buffIndex = 0;
		unsigned blockId = ((unsigned)_buffer[0]) << 16 | (unsigned)(_buffer[1]) << 8 | _buffer[2];
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
	FountainDecoder _decoder;
	std::array<uint8_t, _bufferSize> _buffer;
	unsigned _buffIndex = 0;
};

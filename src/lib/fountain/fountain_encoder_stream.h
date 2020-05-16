#pragma once

#include "FountainEncoder.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class fountain_encoder_stream
{
public:
	static const unsigned _packetSize = 826;

protected:
	fountain_encoder_stream(std::string&& data)
		: _data(data)
		, _encoder((uint8_t*)_data.data(), _data.size())
	{
	}

public:
	template <typename STREAM>
	static fountain_encoder_stream create(STREAM& stream)
	{
		std::stringstream buffs;
		if (stream)
			 buffs << stream.rdbuf();
		return fountain_encoder_stream(buffs.str());
	}

	bool good() const
	{
		return _encoder.good();
	}

	unsigned block_count() const
	{
		return _block;
	}

	unsigned blocks_required() const
	{
		return (_data.size() / _packetSize) + 1;
	}

	void encode_new_block()
	{
		size_t res = _encoder.encode(_block++, _buffer);
		if (res != _buffer.size())
			_encoder.encode(_block++, _buffer); // try twice -- the last initial "packet" will be the wrong size
		_buffIndex = 0;
	}

	// sometimes we want a new encoded batch, sometimes we just want our buffer
	std::streamsize readsome(char* data, unsigned length)
	{
		std::streamsize totalRead = 0;
		while (length > 0 and good())
		{
			if (_buffIndex >= _buffer.size())
				encode_new_block();

			unsigned readLen = std::min(length, (unsigned)(_buffer.size() - _buffIndex));
			if (!readLen)
				return totalRead;
			totalRead += readLen;

			uint8_t* first = _buffer.data() + _buffIndex;
			std::copy(first, first + readLen, data);

			length -= readLen;
			_buffIndex += readLen;
		}
		return totalRead;
	}

protected:
	std::string _data;
	FountainEncoder<_packetSize> _encoder;
	FountainEncoder<_packetSize>::buffer _buffer;
	unsigned _buffIndex = ~0U;
	unsigned _block = 0;
};

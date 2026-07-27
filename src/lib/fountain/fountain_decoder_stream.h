/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "FountainDecoder.h"
#include "FountainMetadata.h"
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

	bool decode()
	{
		// if we're full
		_buffIndex = 0;
		FountainMetadata md(reinterpret_cast<const char*>(_buffer.data()), 6);
		if (data_size() != md.file_size())
			return false; // sanity check
		return _decoder.decode(md.block_id(), _buffer.data() + _headerSize, block_size());
	}

	// roll forward every _bufferSize bytes
	bool write(const char* data, unsigned length)
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
				bool res = decode();
				if (res)
					return res;
			}
		}
		return false;
	}

	bool recover(unsigned char* data, unsigned size)
	{
		return _decoder.recover(data, size);
	}

	std::optional<std::vector<uint8_t>> recover()
	{
		return _decoder.recover();
	}

protected:
	std::vector<uint8_t> _buffer;
	FountainDecoder _decoder;
	unsigned _buffIndex = 0;
};

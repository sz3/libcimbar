#pragma once

#include "FountainInit.h"
#include "wirehair/wirehair.h"
#include <array>
#include <optional>
#include <vector>
#include <iostream>

// will need to split large files

template <size_t _packetSize>
class FountainDecoder
{
public:
	static bool init()
	{
		return FountainInit::init();
	}

public:
	FountainDecoder(size_t length)
	    : _codec(wirehair_decoder_create(nullptr, length, _packetSize))
	    , _length(length)
	{
	}

	~FountainDecoder()
	{
		wirehair_free(_codec);
	}

	bool good() const
	{
		return _codec != nullptr;
	}

	WirehairResult last_result() const
	{
		return _res;
	}

	std::optional<std::vector<uint8_t>> decode(unsigned block_num, uint8_t* data, size_t length)
	{
		_res = wirehair_decode(_codec, block_num, data, length);
		if (_res != Wirehair_Success)
			return std::nullopt;

		// or, we're theoretically done
		std::vector<uint8_t> bytes;
		bytes.resize(_length);
		_res = wirehair_recover(_codec, bytes.data(), bytes.size());
		if (_res != Wirehair_Success)
			return std::nullopt; // :(

		return bytes;
	}

protected:
	WirehairCodec _codec;
	WirehairResult _res;
	size_t _length;
};

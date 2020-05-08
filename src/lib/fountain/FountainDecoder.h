#pragma once

#include "WirehairInit.h"
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
		return WireHairInit::init();
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

	std::optional<std::vector<uint8_t>> decode(unsigned block_num, char* data, size_t length)
	{

		WirehairResult res = wirehair_decode(_codec, block_num, data, length);
		if (res != Wirehair_Success)
		{
			std::cout << "decode got res " << res << std::endl;
			return std::nullopt;
		}

		// or, we're theoretically done
		std::vector<uint8_t> bytes;
		bytes.reserve(_length);
		res = wirehair_recover(_codec, bytes.data(), bytes.size());
		if (res != Wirehair_Success)
			return std::nullopt; // :(

		return bytes;
	}

protected:
	WirehairCodec _codec;
	size_t _length;
};

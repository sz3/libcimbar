#pragma once

#include "FountainInit.h"
#include "wirehair/wirehair.h"
#include <array>
#include <vector>
#include <iostream>

// will need to split large files

template <size_t _packetSize>
class FountainEncoder
{
public:
	using buffer = std::array<uint8_t,_packetSize>;
	static bool init()
	{
		return FountainInit::init();
	}

public:
	FountainEncoder(const uint8_t* data, size_t length)
	    : _codec(wirehair_encoder_create(nullptr, data, length, _packetSize))
	{
	}

	~FountainEncoder()
	{
		wirehair_free(_codec);
	}

	bool good() const
	{
		return _codec != nullptr;
	}

	size_t encode(unsigned block_num, buffer& buff)
	{
		uint32_t written = 0;
		WirehairResult res = wirehair_encode(_codec, block_num, buff.data(), buff.size(), &written);
		if (res != Wirehair_Success)
		{
			std::cout << "wirehair_encode failed: " << res << std::endl;
			return 0;
		}
		return written;
	}

protected:
	WirehairCodec _codec;
};

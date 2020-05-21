#pragma once

#include "FountainInit.h"
#include "wirehair/wirehair.h"
#include <array>
#include <cassert>
#include <vector>
#include <iostream>

// will need to split large files

class FountainEncoder
{
public:
	static bool init()
	{
		return FountainInit::init();
	}

public:
	FountainEncoder(const uint8_t* data, size_t length, size_t packet_size)
	    : _codec(wirehair_encoder_create(nullptr, data, length, packet_size))
	    , _packetSize(packet_size)
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

	size_t encode(unsigned block_num, uint8_t* buff, size_t size)
	{
		assert(size == _packetSize);
		uint32_t written = 0;
		WirehairResult res = wirehair_encode(_codec, block_num, buff, size, &written);
		if (res != Wirehair_Success)
			return 0;
		return written;
	}

	size_t packet_size() const
	{
		return _packetSize;
	}

protected:
	WirehairCodec _codec;
	size_t _packetSize;
};

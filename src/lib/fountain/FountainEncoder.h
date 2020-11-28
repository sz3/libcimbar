/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "FountainInit.h"
#include "wirehair/wirehair.h"
#include <cassert>

class FountainEncoder
{
protected:
	void swap(FountainEncoder& other) throw()
	{
		std::swap(_codec, other._codec);
		std::swap(_packetSize, other._packetSize);
	}

public:
	FountainEncoder(const uint8_t* data, size_t length, size_t packet_size)
	    : _packetSize(packet_size)
	{
		FountainInit::init();
		_codec = wirehair_encoder_create(nullptr, data, length, packet_size);
	}

	~FountainEncoder()
	{
		wirehair_free(_codec);
	}

	FountainEncoder& operator=(FountainEncoder temp)
	{
		temp.swap(*this);
		return *this;
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

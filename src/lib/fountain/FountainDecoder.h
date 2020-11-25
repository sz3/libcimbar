/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "FountainInit.h"
#include "wirehair/wirehair.h"
#include <array>
#include <optional>
#include <set>
#include <vector>
#include <iostream>

// will need to split large files

class FountainDecoder
{
public:
	FountainDecoder(size_t length, size_t packet_size)
	    : _length(length)
	{
		FountainInit::init();
		_codec = wirehair_decoder_create(nullptr, length, packet_size);
	}

	~FountainDecoder()
	{
		wirehair_free(_codec);
	}

	unsigned progress() const
	{
		return _seenBlocks.size();
	}

	size_t length() const
	{
		return _length;
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
		auto pear = _seenBlocks.insert(block_num);
		if (!pear.second)
			return std::nullopt;

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
	std::set<unsigned> _seenBlocks; // giving wirehair_decode the same block too many times can make it very, very upset
};

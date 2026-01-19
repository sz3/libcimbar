/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "FountainInit.h"
#include "wirehair/wirehair.h"
#include <array>
#include <optional>
#include <set>
#include <vector>

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

	bool decode(unsigned block_num, uint8_t* data, size_t length)
	{
		auto pear = _seenBlocks.insert(block_num);
		if (!pear.second)
			return false;

		_res = wirehair_decode(_codec, block_num, data, length);
		if (_res != Wirehair_Success)
			return false;

		// we're theoretically done
		return true;
	}

	bool recover(unsigned char* data, unsigned size)
	{
		_res = wirehair_recover(_codec, data, size);
		return _res == Wirehair_Success;
	}

	std::optional<std::vector<uint8_t>> recover()
	{
		std::vector<uint8_t> bytes;
		bytes.resize(_length);
		if (!recover(bytes.data(), bytes.size()))
			return std::nullopt; // :(

		return bytes;
	}

protected:
	WirehairCodec _codec;
	WirehairResult _res;
	size_t _length;
	std::set<unsigned> _seenBlocks; // giving wirehair_decode the same block too many times can make it very, very upset
};

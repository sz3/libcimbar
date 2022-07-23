/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <iostream>

template<typename C, size_t N, size_t READLEN=8>
class bit_extractor
{
protected:
	static constexpr uint64_t BITMASK = (1 << READLEN) - 1; // e.g. 0xFF, 0x1F, 0xF

public:
	bit_extractor(const C& bits)
		: _bits(bits)
	{}

	template<typename... T>
	uint64_t extract()
	{
		return 0;
	}

	template<typename... T>
	uint64_t extract(unsigned bit_offset, const T&... t)
	{
		constexpr auto byte_offset = sizeof...(T);

		uint64_t total = ((uint64_t)(_bits >> (N - bit_offset - READLEN)) & BITMASK) << (byte_offset * READLEN); // if byte_offset is 1, we want to shift 5. if 2, 10....
		return total | extract(t...);
	}

protected:
	C _bits;
};

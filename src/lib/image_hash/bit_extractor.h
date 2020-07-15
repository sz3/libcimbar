#pragma once

#include <iostream>

template<typename C, size_t N>
class bit_extractor
{
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

		uint64_t total = ((uint64_t)(_bits >> (N - bit_offset - 8)) & 0xFF) << (byte_offset << 3); // if byte_offset is 1, we want to shift 8. if 2, 16....
		return total | extract(t...);
	}

protected:
	C _bits;
};

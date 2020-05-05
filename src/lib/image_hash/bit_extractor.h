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
	uint64_t extract(unsigned offset, const T&... t)
	{
		constexpr auto size = sizeof...(T);

		uint64_t total = ((uint64_t)(_bits >> (N - offset - 8)) & 0xFF) << (size << 3); // 3 == number of bits. 8 == 2^3.
		return total | extract(t...);
	}

protected:
	C _bits;
};

#pragma once

#include <bitset>
#include <iostream>

template<typename C>
class bitset_extractor
{
public:
	bitset_extractor(const C& bits)
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

		uint64_t total = ((_bits << offset) >> (_bits.size() - 8)).to_ullong() << (size << 3); // 3 == number of bits. 8 == 2^3.
		return total + extract(t...);
	}

protected:
	C _bits;
};

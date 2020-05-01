#pragma once

#include <bitset>
#include <iostream>

template<size_t N>
class bitset_extractor
{
public:
	bitset_extractor(const std::bitset<N>& bits)
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

		uint64_t total = ((_bits << offset) >> (N - 8)).to_ullong() << (size << 3); // 3 == number of bits. 8 == 2^3.
		return total + extract(t...);
	}

protected:
	std::bitset<N> _bits;
};

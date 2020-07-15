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

	template<typename... T>
	void extract64_internal(uint64_t& total)
	{
		return;
	}

	// need to compile-time detect/use endianness if this is going to be useful
	template<typename... T>
	void extract64_internal(uint64_t& total, unsigned bit_offset, const T&... t)
	{
		constexpr auto byte_offset = sizeof...(T);

		uint8_t* loc = reinterpret_cast<uint8_t*>(&total) + byte_offset;
		*loc = (uint8_t)(_bits >> (N - bit_offset - 8));
		extract64_internal(total, t...);
	}

	template<typename... T>
	uint64_t extract64(const T&... t)
	{
		uint64_t total = 0;
		extract64_internal(total, t...);
		return total;
	}

protected:
	C _bits;
};

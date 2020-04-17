#pragma once

namespace image_hash
{
	// https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
	template <typename Integer>
	inline unsigned hamming_weight(Integer v)
	{
		unsigned count = 0; // c accumulates the total bits set in v
		while (v)
		{
			++count;
			v &= (v - 1); // clear the least significant bit set
		}
		return count;
	}

	template <typename Integer>
	inline unsigned hamming_distance(Integer a, Integer b)
	{
		return hamming_weight(a xor b);
	}
}

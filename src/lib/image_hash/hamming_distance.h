#pragma once

#include "libpopcnt/libpopcnt.h"

namespace image_hash
{
	template <typename Integer>
	inline unsigned hamming_distance(Integer a, Integer b)
	{
		return popcnt64(a xor b);
	}
}

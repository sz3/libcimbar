/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
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

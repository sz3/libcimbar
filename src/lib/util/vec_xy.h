/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

namespace cimbar {

struct vec_xy
{
	unsigned x = 0;
	unsigned y = 0;

	unsigned width() const
	{
		return x;
	};

	unsigned height() const
	{
		return y;
	}
};

}

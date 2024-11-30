/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

namespace cimbar {

struct vec_xy
{
	int x = 0;
	int y = 0;

	int width() const
	{
		return x;
	};

	int height() const
	{
		return y;
	}
};

}

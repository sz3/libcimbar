/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <cassert>
#include <string>

namespace cimbar {

template <typename IMG>
struct load
{
	static IMG load_img(std::string path)
	{
		assert(false);
	}

	static IMG getTile(unsigned symbol_bits, unsigned symbol, bool dark=true, unsigned num_colors=4, unsigned color=0)
	{
		assert(false);
	}
};


}

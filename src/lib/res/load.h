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

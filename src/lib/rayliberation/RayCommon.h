/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "res/load.h"
#include <string>

namespace cimbar {

class texture;

template <>
struct load<texture>
{
	static texture load_img(std::string path);
	static texture getTile(unsigned symbol_bits, unsigned symbol, bool dark=true, unsigned num_colors=4, unsigned color=0);
};

}

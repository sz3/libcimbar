/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <string>
#include <tuple>

namespace cimbar
{
	using RGB = std::tuple<unsigned char, unsigned char, unsigned char>;

	std::string load_file(std::string path);
	std::tuple<unsigned char, unsigned char, unsigned char> getColor(unsigned index, unsigned num_colors);
}

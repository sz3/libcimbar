/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "res.h"

#include "base91/base.hpp"
#include "serialize/format.h"

#include <array>
#include <map>
#include <string>
#include "bitmaps.h"

using cimbar::RGB;
using std::array;
using std::string;

namespace {
	RGB getColor4(unsigned index)
	{
		// opencv uses BGR, but we don't have to conform to this tyranny
		static constexpr array<RGB, 4> colors = {
		    RGB(0, 0xFF, 0xFF),
		    RGB(0xFF, 0xFF, 0),
		    RGB(0xFF, 0, 0xFF),
		    RGB(0, 0xFF, 0)
		};
		return colors[index];
	}

	RGB getColor8(unsigned index)
	{
		static constexpr array<RGB, 8> colors = {
		    RGB(0, 0xFF, 0xFF), // cyan
		    RGB(0x7F, 0x7F, 0xFF),  // mid-blue
		    RGB(0xFF, 0, 0xFF), // magenta
		    RGB(0xFF, 65, 65), // red
		    RGB(0xFF, 0x9F, 0),  // orange
		    RGB(0xFF, 0xFF, 0), // yellow
		    RGB(0xFF, 0xFF, 0xFF),
		    RGB(0, 0xFF, 0),
		};
		return colors[index];
	}

}

namespace cimbar {

string load_file(string path)
{
	auto it = cimbar::bitmaps.find(path);
	if (it == cimbar::bitmaps.end())
		return "";

	return base91::decode(it->second);
}

RGB getColor(unsigned index, unsigned num_colors)
{
	if (num_colors <= 4)
		return getColor4(index);
	else
		return getColor8(index);
}

}

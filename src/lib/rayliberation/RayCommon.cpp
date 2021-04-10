/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "RayCommon.h"
#include "cimb_translator/res.h"

#include "base91/base.hpp"
#include "serialize/format.h"
#include "rayliberation/texture.h"

using std::string;

namespace cimbar {

cimbar::texture load_img(string path)
{
	string bytes = load_file(path);
	if (bytes.empty())
		return cimbar::texture();

	return cimbar::texture(LoadImageFromMemory("png", reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size()), WHITE);
}

cimbar::texture getTile(unsigned symbol_bits, unsigned symbol, bool dark=true, unsigned num_colors=4, unsigned color=0)
{
	string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cimbar::texture tile = load_img(imgPath);

	uchar r, g, b;
	std::tie(r, g, b) = getColor(color, num_colors);

	tile.set_tint({r, g, b, 255});
	return tile;
}

}

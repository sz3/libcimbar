
#include "RayCommon.h"
#include "cimb_translator/res.h"
#include "serialize/format.h"

#include "raylib.h"
#include <string>

texture load_img(string path)
{
	std::string bytes = load_file(path);
	if (bytes.empty())
		return cimbar::texture();

	return cimbar::texture(LoadImageFromMemory("png", reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size()), WHITE);
}

texture getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned num_colors, unsigned color)
{
	std::string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cimbar::texture tile = load_texture(imgPath);

	unsigned char r, g, b;
	std::tie(r, g, b) = getColor(color, num_colors);

	tile.set_tint({r, g, b, 255});
	return tile;
}

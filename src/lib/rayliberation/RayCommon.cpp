
#include "RayCommon.h"
#include "texture.h"

#include "res/res.h"
#include "serialize/format.h"

#include "raylib.h"
#include <string>

using std::string;

namespace cimbar {

texture load_img(string path)
{
	std::string bytes = load_file(path);
	if (bytes.empty())
		return cimbar::texture();

	Image img = LoadImageFromMemory("png", reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
	cimbar::texture tx(img, WHITE);
	UnloadImage(img);
	return tx;
}

texture getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned num_colors, unsigned color)
{
	std::string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cimbar::texture tile = load_img(imgPath);

	unsigned char r, g, b;
	std::tie(r, g, b) = getColor(color, num_colors);

	tile.set_tint({r, g, b, 255});
	return tile;
}

}

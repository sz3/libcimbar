
#include "RayCommon.h"
#include "texture.h"
#include "types.h"

#include "res/res.h"
#include "serialize/format.h"

#include "raylib.h"
#include <string>
#include <unordered_map>

using std::string;

namespace {
	std::shared_ptr<raylibpp::render_texture> _frame;
	std::unordered_map<string, cimbar::texture> _tileMap;
}

namespace cimbar {

std::shared_ptr<raylibpp::render_texture> init_frame(int width, int height)
{
	if (!_frame)
		_frame = std::make_shared<raylibpp::render_texture>(LoadRenderTexture(width, height));
	return _frame;
}

texture load<texture>::load_img(string path)
{
	std::unordered_map<string, cimbar::texture>::const_iterator it = _tileMap.find(path);
	if (it != _tileMap.end())
		return it->second;

	std::string bytes = load_file(path);
	if (bytes.empty())
		return cimbar::texture();

	Image img = LoadImageFromMemory("png", reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
	_tileMap[path] = cimbar::texture(img, WHITE);
	UnloadImage(img);
	return _tileMap[path];
}

texture load<texture>::getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned num_colors, unsigned color)
{
	std::string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cimbar::texture tile = load_img(imgPath);

	unsigned char r, g, b;
	std::tie(r, g, b) = getColor(color, num_colors);

	tile.set_tint({r, g, b, 255});
	return tile;
}

}

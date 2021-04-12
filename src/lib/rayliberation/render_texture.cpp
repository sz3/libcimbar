#include "render_texture.h"

#include "texture.h"
#include <iostream>

using std::get;

namespace cimbar {

render_texture::render_texture(unsigned width, unsigned height, Color bgcolor)
    : _rtex(std::make_shared<raylibpp::render_texture>(LoadRenderTexture(width, height)))
    , _bg(bgcolor)
{
	clear();
}

render_texture::render_texture(int width, int height, int, std::tuple<unsigned char, unsigned char, unsigned char> color)
    : render_texture(width, height, {get<0>(color), get<1>(color), get<2>(color), 255})
{
}

void render_texture::clear()
{
	BeginTextureMode(_rtex->tx);
	ClearBackground(_bg);
	EndTextureMode();
}

void render_texture::paste(const texture& tx, int x, int y)
{
	BeginTextureMode(_rtex->tx);
	DrawTexture(tx.tx(), x, y, tx.tint());
	EndTextureMode();
}

void render_texture::draw(int x, int y) const
{
	DrawTextureRec(_rtex->tx.texture, {x, y, static_cast<float>(_rtex->tx.texture.width), static_cast<float>(-_rtex->tx.texture.height)}, {0, 0}, WHITE);
}

render_view render_texture::operator()(std::tuple<int,int,int,int> params) // matching cv::Mat interface for now...
{
	return render_view({*this, std::get<0>(params), std::get<1>(params)});
}

Image render_texture::screenshot() const
{
	return GetTextureData(_rtex->tx.texture);
}

}

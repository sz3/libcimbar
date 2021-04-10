#include "render_texture.h"

#include "texture.h"
#include <tuple>
#include <iostream>

namespace cimbar {


render_texture::render_texture(unsigned width, unsigned height, Color bgcolor)
    : _rtex(LoadRenderTexture(width, height))
    , _bg(bgcolor)
{
	clear();
}

render_texture::render_texture(int width, int height, int, cv::Scalar color)
    : render_texture(width, height, {color[0], color[1], color[2], 255})
{
}

void render_texture::clear()
{
	BeginTextureMode(_rtex);
	ClearBackground(_bg);
	EndTextureMode();
}

void render_texture::paste(const texture& tx, int x, int y)
{
	BeginTextureMode(_rtex);
	DrawTexture(tx.tx(), x, y, tx.tint());
	EndTextureMode();
}

void render_texture::draw()
{
	DrawTextureRec(_rtex.texture, {0, 0, static_cast<float>(_rtex.texture.width), static_cast<float>(-_rtex.texture.height)}, {0, 0}, WHITE);
}

render_texture::slice render_texture::operator()(std::tuple<int,int,int,int> params) // matching cv::Mat interface for now...
{
	return slice({*this, std::get<0>(params), std::get<1>(params)});
}

Image render_texture::screenshot() const
{
	GetTextureData(_rtex.texture);
}

}

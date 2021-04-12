#include "render_texture.h"

#include "RayCommon.h"
#include "texture.h"
#include <iostream>

using std::get;

namespace cimbar {

render_texture::render_texture(int width, int height, Color bgcolor)
	: _rtex(init_frame(width, height))
	, _bg(bgcolor)
{
	begin();
}

render_texture::render_texture(int width, int height, int, std::tuple<unsigned char, unsigned char, unsigned char> color)
	: render_texture(width, height, {get<0>(color), get<1>(color), get<2>(color), 255})
{
	begin();
}

render_texture::~render_texture()
{
	end();
}

void render_texture::begin()
{
	BeginTextureMode(_rtex->tx);
	ClearBackground(_bg);
}

void render_texture::end()
{
	EndTextureMode();
}

void render_texture::clear()
{
	ClearBackground(_bg);
}

void render_texture::paste(const texture& tx, int x, int y)
{
	DrawTexture(tx.tx(), x, y, tx.tint());
}

void render_texture::draw(int x, int y) const
{
	DrawTextureRec(_rtex->tx.texture, {0, 0, static_cast<float>(_rtex->tx.texture.width), static_cast<float>(-_rtex->tx.texture.height)}, {x, y}, WHITE);
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

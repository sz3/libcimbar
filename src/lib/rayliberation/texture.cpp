#include "raylib.h"

#include "texture.h"
#include "render_texture.h"
#include "serialize/format.h"

#include "raylib.h"
#include <iostream>

namespace cimbar {

texture::texture()
    : _tx{0, 0, 0, 0, 0}
    , _tint(WHITE)
{
}

texture::texture(const Image& img, Color tint)
    : _tx(LoadTextureFromImage(img))
    , _tint(tint)
    , rows(_tx.height)
    , cols(_tx.width)
{
}

// TODO: destructor!

texture::operator bool() const
{
	return _tx.width > 0;
}

const Texture2D& texture::tx() const
{
	return _tx;
}

void texture::set_tint(Color c)
{
	std::cout << fmt::format("set tint {},{},{},{}", c.a, c.r, c.g, c.b) << std::endl;
	_tint = c;
}

const Color& texture::tint() const
{
	return _tint;
}

void texture::copyTo(render_view slice) const
{
	slice.rtx.paste(*this, slice.x, slice.y);
}

Image texture::screenshot() const
{
	return GetTextureData(_tx);
}


}

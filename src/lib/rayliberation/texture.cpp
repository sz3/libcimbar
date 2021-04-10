#include "raylib.h"

#include "texture.h"
#include "render_texture.h"

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
	_tint = c;
}

const Color& texture::tint() const
{
	return _tint;
}

void texture::copyTo(render_texture::slice slice)
{
	slice.rtx.paste(*this, slice.x, slice.y);
}

}

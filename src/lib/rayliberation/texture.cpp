#include "raylib.h"

#include "texture.h"
#include "render_texture.h"
#include "serialize/format.h"

#include "raylib.h"
#include <iostream>

namespace cimbar {

texture::texture()
    : _tx(nullptr)
    , _tint(WHITE)
{
}

texture::texture(const Image& img, Color tint)
    : _tx(std::make_shared<raylibpp::texture>(LoadTextureFromImage(img)))
    , _tint(tint)
    , rows(_tx->tx.height)
    , cols(_tx->tx.width)
{
}

texture::operator bool() const
{
	return !!_tx;
}

const Texture2D& texture::tx() const
{
	return _tx->tx;
}

void texture::set_tint(Color c)
{
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
	return GetTextureData(_tx->tx);
}


}

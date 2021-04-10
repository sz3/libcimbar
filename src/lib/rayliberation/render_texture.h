#pragma once

#include "texture.h"
#include <tuple>
#include <iostream>

namespace cimbar {

class render_texture
{
// class slice?
// need a way to make texture::copyTo(...) work...

public:
	render_texture(unsigned width, unsigned height, Color bgcolor)
		: _rtex(LoadRenderTexture(width, height))
		, _bg(bgcolor)
	{
		clear();
	}

	void clear()
	{
		BeginTextureMode(_rtex);
		ClearBackground(_bg);
		EndTextureMode();
	}

	void paste(const texture& tx, int x, int y)
	{
		BeginTextureMode(_rtex);
		DrawTexture(tx.tx(), x, y, tx.tint());
		EndTextureMode();
	}

	void draw()
	{
		DrawTextureRec(_rtex.texture, {0, 0, static_cast<float>(_rtex.texture.width), static_cast<float>(-_rtex.texture.height)}, {0, 0}, WHITE);
	}

protected:
	RenderTexture2D _rtex;
	Color _bg;
};

}

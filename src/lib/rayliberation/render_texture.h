#pragma once

#include "texture.h"
#include <tuple>

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
		BeginTextureMode(_rtex);
		ClearBackground(_bg);
	}

	void paste(const texture& tx, int x, int y, Color tint)
	{
		DrawTexture(tx.tx(), x, y, tx.tint());
	}

	void draw()
	{
		EndTextureMode();
		DrawTexture(_rtex.texture, 0, 0, WHITE);
	}

protected:
	RenderTexture2D _rtex;
	Color _bg;
};

}

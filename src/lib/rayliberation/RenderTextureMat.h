#pragma once

#include "TextureMat.h"
#include <tuple>

namespace cimbar {

class RenderTextureMat
{
// class slice?
// need a way to make TextureMat::copyTo(...) work...

public:
	RenderTextureMat(unsigned width, unsigned height, Color bgcolor)
	    : _rtex(LoadRenderTexture(width, height))
	    , _bg(bgcolor)
	{
		BeginTextureMode(_rtex);
		ClearBackground(_bg);
	}

	void paste(const TextureMat& tx, int x, int y, Color tint)
	{
		DrawTexture(tx.texture(), x, y, tx.tint());
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

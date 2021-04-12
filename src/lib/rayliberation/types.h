#pragma once

#include "raylib.h"

namespace raylibpp {

	struct texture
	{
		texture(Texture2D tx)
			: tx(tx)
		{}

		~texture()
		{
			UnloadTexture(tx);
		}

		Texture2D tx;
	};

	struct render_texture
	{
		render_texture(RenderTexture2D tx)
			: tx(tx)
		{}

		~render_texture()
		{
			UnloadRenderTexture(tx);
		}

		RenderTexture2D tx;
	};
}

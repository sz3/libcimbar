#pragma once

#include "raylib.h"
#include <tuple>

namespace cimbar {

class texture;

class render_texture
{
public:
	// for copyTo()
	struct slice
	{
		render_texture& rtx;
		int x;
		int y;
	};

public:
	render_texture(unsigned width, unsigned height, Color bgcolor);

	void clear();
	void paste(const texture& tx, int x, int y);
	void draw();

	slice operator()(std::tuple<int,int,int,int> params);

protected:
	RenderTexture2D _rtex;
	Color _bg;
};

}

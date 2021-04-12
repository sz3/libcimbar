/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "rayliberation/render_texture.h"
#include "util/loop_iterator.h"

#include <iostream>
#include <string>

namespace cimbar {

class window_raylib
{
protected:
	static constexpr std::array<std::pair<int, int>, 4> SHAKE_POS = {{
	    {0, 0}, {-8, -8}, {0, 0}, {8, 8}
	}};

public:
	window_raylib(unsigned width, unsigned height, std::string title)
	    : _shake(SHAKE_POS)
	    , _offset({(width - 1024)/2, (height - 1024)/2})
	{
		InitWindow(width, height, title.c_str());
	}

	~window_raylib()
	{
		CloseWindow();
	}

	bool is_good() const
	{
		return true;
	}

	bool should_close() const
	{
		return WindowShouldClose();
	}

	void shake(unsigned i=1)
	{
		if (i == 0)
			_shake.reset();
		else
			++_shake;
	}

	void clear()
	{
		BeginDrawing();
		ClearBackground({0, 0, 0, 0});
		EndDrawing();
	}

	void show(cimbar::render_texture& img, unsigned delay)
	{
		BeginDrawing();
		ClearBackground(BLACK); // locked into dark mode for now?
		img.draw((*_shake).first + _offset.first, (*_shake).second + _offset.second);
		EndDrawing();
	}

	unsigned width() const
	{
		return 1024;
	}

protected:
	loop_iterator<decltype(SHAKE_POS)> _shake;
	std::pair<int, int> _offset;
};

}

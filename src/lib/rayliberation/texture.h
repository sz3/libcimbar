#pragma once

#include "raylib.h"

namespace cimbar {

class render_texture;
class render_view;

class texture
{
public:
	texture();
	texture(const Image& img, Color tint);

	operator bool() const;

	const Texture2D& tx() const;

	void set_tint(Color c);
	const Color& tint() const;

	void copyTo(render_view slice) const;

	Image screenshot() const;

protected:
	Texture2D _tx;
	Color _tint;

public:
	int rows = 0; // matching cv::Mat interface for now..
	int cols = 0;
};

}

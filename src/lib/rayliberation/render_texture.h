#pragma once

#include "raylib.h"
#include <opencv2/core/types.hpp>
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
	render_texture(int width, int height, int, cv::Scalar color); // match cv::Mat

	void clear();
	void paste(const texture& tx, int x, int y);
	void draw();

	Image screenshot() const;

	slice operator()(std::tuple<int,int,int,int> params);

protected:
	RenderTexture2D _rtex;
	Color _bg;
};

}

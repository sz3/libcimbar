/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CellPositions.h"
#include "CimbEncoder.h"
#include "util/vec_xy.h"

class CimbWriter
{
public:
	CimbWriter(unsigned symbol_bits=0, int color_bits=-1, bool dark=true, unsigned color_mode=1, cimbar::vec_xy size={});
	void set_color_mode(unsigned color_mode);

	bool write(unsigned bits);
	bool done() const;

	cv::Mat image() const;

	unsigned num_cells() const;

protected:
	void paste(const cv::Mat& img, int x, int y);

protected:
	cv::Mat _image;
	CellPositions _positions;
	CimbEncoder _encoder;
	unsigned _offsetX = 0;
	unsigned _offsetY = 0;
};

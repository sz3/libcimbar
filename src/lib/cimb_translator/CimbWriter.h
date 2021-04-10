/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CellPositions.h"
#include "CimbEncoder.h"
#include "graphics/image.h"

class CimbWriter
{
public:
	CimbWriter(unsigned symbol_bits, unsigned color_bits, bool dark=true, int size=0);

	bool write(unsigned bits);
	bool done() const;

	cv::Mat image() const;

protected:
	void paste(const cimbar::image& img, int x, int y);

protected:
	cimbar::image _image;
	CellPositions _positions;
	CimbEncoder _encoder;
	unsigned _offset = 0;
};

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CellPositions.h"
#include "CimbEncoder.h"
#include "Config.h"
#include "serialize/format.h"

#include <string>

template <typename IMG, typename FRAME=IMG>
class CimbWriter
{
// static helpers
protected:
	static IMG getAnchor(bool dark)
	{
		std::string name = dark? "anchor-dark" : "anchor-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	static IMG getSecondaryAnchor(bool dark)
	{
		std::string name = dark? "anchor-secondary-dark" : "anchor-secondary-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	static IMG getHorizontalGuide(bool dark)
	{
		std::string name = dark? "guide-horizontal-dark" : "guide-horizontal-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	static IMG getVerticalGuide(bool dark)
	{
		std::string name = dark? "guide-vertical-dark" : "guide-vertical-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	static int calc_size(int size)
	{
		if (size < cimbar::Config::image_size())
			return cimbar::Config::image_size();
		return size;
	}

// interface
public:
	CimbWriter(unsigned symbol_bits, unsigned color_bits, bool dark=true, int size=0)
	    : _image(calc_size(size), calc_size(size), CV_8UC3, {0, 0, 0})
	    , _positions(cimbar::Config::cell_spacing(), cimbar::Config::num_cells(), cimbar::Config::cell_size(),
	                 cimbar::Config::corner_padding(), cimbar::Config::interleave_blocks(), cimbar::Config::interleave_partitions())
	    , _encoder(symbol_bits, color_bits)
	{
		if (size > cimbar::Config::image_size())
			_offset = (size - cimbar::Config::image_size()) / 2;

		// from here on, we only care about the internal size
		size = cimbar::Config::image_size();

		IMG anchor = getAnchor(dark);
		paste(anchor, 0, 0);
		paste(anchor, 0, size - anchor.cols);
		paste(anchor, size - anchor.rows, 0);

		IMG secondaryAnchor = getSecondaryAnchor(dark);
		paste(secondaryAnchor, size - anchor.rows, size - anchor.cols);

		IMG hg = getHorizontalGuide(dark);
		paste(hg, (size/2) - (hg.cols/2), 2);
		paste(hg, (size/2) - (hg.cols/2), size-4);
		paste(hg, (size/2) - (hg.cols/2) - hg.cols, size-4);
		paste(hg, (size/2) - (hg.cols/2) + hg.cols, size-4);

		IMG vg = getVerticalGuide(dark);
		paste(vg, 2, (size/2) - (vg.rows/2));
		paste(vg, size-4, (size/2) - (vg.rows/2));
	}

	bool write(unsigned bits)
	{
		// check with _encoder for tile, then place it in template according to mapping
		// mapping will track current index/location?
		if (done())
			return false;

		CellPositions::coordinate xy = _positions.next();
		IMG cell = _encoder.encode(bits);
		paste(cell, xy.first, xy.second);
		return true;
	}

	bool done() const
	{
		return _positions.done();
	}

	FRAME image() const
	{
		return _image;
	}

protected:
	void paste(const IMG& img, int x, int y)
	{
		img.copyTo(_image({x+_offset, y+_offset, img.cols, img.rows}));
	}

protected:
	FRAME _image;
	CellPositions _positions;
	CimbEncoder<IMG> _encoder;
	int _offset = 0; // TODO: get rid of this?
};

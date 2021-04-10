/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbWriter.h"

#include "Config.h"
#include "graphics/Common.h"
#include "serialize/format.h"
#include <string>
using std::string;

using namespace cimbar;

namespace {
	cimbar::image getAnchor(bool dark)
	{
		string name = dark? "anchor-dark" : "anchor-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	cimbar::image getSecondaryAnchor(bool dark)
	{
		string name = dark? "anchor-secondary-dark" : "anchor-secondary-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	cimbar::image getHorizontalGuide(bool dark)
	{
		string name = dark? "guide-horizontal-dark" : "guide-horizontal-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	cimbar::image getVerticalGuide(bool dark)
	{
		string name = dark? "guide-vertical-dark" : "guide-vertical-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}
}

CimbWriter::CimbWriter(unsigned symbol_bits, unsigned color_bits, bool dark, int size)
    : _positions(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding(), Config::interleave_blocks(), Config::interleave_partitions())
    , _encoder(symbol_bits, color_bits)
{
	if (size > cimbar::Config::image_size())
		_offset = (size - cimbar::Config::image_size()) / 2;
	else
		size = cimbar::Config::image_size();

	cv::Scalar bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
	_image = cimbar::image(size, size, CV_8UC3, bgcolor);

	// from here on, we only care about the internal size
	size = cimbar::Config::image_size();

	cimbar::image anchor = getAnchor(dark);
	paste(anchor, 0, 0);
	paste(anchor, 0, size - anchor.cols);
	paste(anchor, size - anchor.rows, 0);

	cimbar::image secondaryAnchor = getSecondaryAnchor(dark);
	paste(secondaryAnchor, size - anchor.rows, size - anchor.cols);

	cimbar::image hg = getHorizontalGuide(dark);
	paste(hg, (size/2) - (hg.cols/2), 2);
	paste(hg, (size/2) - (hg.cols/2), size-4);
	paste(hg, (size/2) - (hg.cols/2) - hg.cols, size-4);
	paste(hg, (size/2) - (hg.cols/2) + hg.cols, size-4);

	cimbar::image vg = getVerticalGuide(dark);
	paste(vg, 2, (size/2) - (vg.rows/2));
	paste(vg, size-4, (size/2) - (vg.rows/2));
}

void CimbWriter::paste(const cimbar::image& img, int x, int y)
{
	img.copyTo(_image(cv::Rect(x+_offset, y+_offset, img.cols, img.rows)));
}

bool CimbWriter::write(unsigned bits)
{
	// check with _encoder for tile, then place it in template according to mapping
	// mapping will track current index/location?
	if (done())
		return false;

	CellPositions::coordinate xy = _positions.next();
	cimbar::image cell = _encoder.encode(bits);
	paste(cell, xy.first, xy.second);
	return true;
}

bool CimbWriter::done() const
{
	return _positions.done();
}

cimbar::image CimbWriter::image() const
{
	return _image;
}

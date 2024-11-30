/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbWriter.h"

#include "Common.h"
#include "Config.h"
#include "serialize/format.h"
#include <string>
#include <iostream>
using std::string;

using namespace cimbar;

namespace {
	cv::Mat getAnchor(bool dark)
	{
		string name = dark? "anchor-dark" : "anchor-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	cv::Mat getSecondaryAnchor(bool dark)
	{
		string name = dark? "anchor-secondary-dark" : "anchor-secondary-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	cv::Mat getHorizontalGuide(bool dark)
	{
		string name = dark? "guide-horizontal-dark" : "guide-horizontal-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}

	cv::Mat getVerticalGuide(bool dark)
	{
		string name = dark? "guide-vertical-dark" : "guide-vertical-light";
		return cimbar::load_img(fmt::format("bitmap/{}.png", name));
	}
}

CimbWriter::CimbWriter(unsigned symbol_bits, unsigned color_bits, bool dark, unsigned color_mode, vec_xy size)
	: _positions(Config::cell_spacing_x(), Config::cell_spacing_y(), Config::cells_per_col_x(), Config::cells_per_col_y(), Config::cell_offset(), Config::corner_padding_x(), Config::corner_padding_y(), Config::interleave_blocks(), Config::interleave_partitions())
	, _encoder(symbol_bits, color_bits, dark, color_mode)
{
	int height = std::max(size.height(), cimbar::Config::image_size_y());
	int width = std::max(size.width(), cimbar::Config::image_size_x());

	_offsetX = (width - cimbar::Config::image_size_x()) / 2;
	_offsetY = (height - cimbar::Config::image_size_y()) / 2;

	cv::Scalar bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
	_image = cv::Mat(height, width, CV_8UC3, bgcolor);

	// from here on, we only care about the internal size
	width = cimbar::Config::image_size_x();
	height = cimbar::Config::image_size_y();

	cv::Mat anchor = getAnchor(dark);
	paste(anchor, 0, 0);
	paste(anchor, 0, height - anchor.rows);
	paste(anchor, width - anchor.cols, 0);

	cv::Mat secondaryAnchor = getSecondaryAnchor(dark);
	paste(secondaryAnchor, width - anchor.cols, height - anchor.rows);

	cv::Mat hg = getHorizontalGuide(dark);
	paste(hg, (width/2) - (hg.cols/2), 2);
	paste(hg, (width/2) - (hg.cols/2), height-4);
	paste(hg, (width/2) - (hg.cols/2) - hg.cols, height-4);
	paste(hg, (width/2) - (hg.cols/2) + hg.cols, height-4);

	cv::Mat vg = getVerticalGuide(dark);
	paste(vg, 2, (height/2) - (vg.rows/2));
	paste(vg, width-4, (height/2) - (vg.rows/2));
}

void CimbWriter::paste(const cv::Mat& img, int x, int y)
{
	img.copyTo(_image(cv::Rect(x+_offsetX, y+_offsetY, img.cols, img.rows)));
}

bool CimbWriter::write(unsigned bits)
{
	// check with _encoder for tile, then place it in template according to mapping
	// mapping will track current index/location?
	if (done())
		return false;

	CellPositions::coordinate xy = _positions.next();
	cv::Mat cell = _encoder.encode(bits);
	paste(cell, xy.first, xy.second);
	return true;
}

bool CimbWriter::done() const
{
	return _positions.done();
}

cv::Mat CimbWriter::image() const
{
	return _image;
}

unsigned CimbWriter::num_cells() const
{
	return _positions.count();
}

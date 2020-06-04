#include "CimbWriter.h"

#include "Common.h"
#include "Config.h"
#include "serialize/format.h"
#include <string>
using std::string;

using namespace cimbar;

namespace {
	cv::Mat getAnchor(bool dark)
	{
		string name = dark? "anchor-dark" : "anchor-light";
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

	void paste(cv::Mat& canvas, const cv::Mat& img, int x, int y)
	{
		img.copyTo(canvas(cv::Rect(x, y, img.cols, img.rows)));
	}
}

CimbWriter::CimbWriter(bool dark, unsigned size)
    : _position(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding(), Config::interleave_blocks())
    , _encoder(Config::symbol_bits(), Config::color_bits())
{
	cv::Scalar bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
	_image = cv::Mat(size, size, CV_8UC3, bgcolor);

	cv::Mat anchor = getAnchor(dark);
	paste(_image, anchor, 0, 0);
	paste(_image, anchor, 0, size - anchor.cols);
	paste(_image, anchor, size - anchor.rows, 0);
	paste(_image, anchor, size - anchor.rows, size - anchor.cols);

	cv::Mat hg = getHorizontalGuide(dark);
	paste(_image, hg, (size/2) - (hg.cols/2), 2);
	paste(_image, hg, (size/2) - (hg.cols/2), size-4);
	paste(_image, hg, (size/2) - (hg.cols/2) - hg.cols, size-4);
	paste(_image, hg, (size/2) - (hg.cols/2) + hg.cols, size-4);

	cv::Mat vg = getVerticalGuide(dark);
	paste(_image, vg, 2, (size/2) - (vg.rows/2));
	paste(_image, vg, size-4, (size/2) - (vg.rows/2));
}

bool CimbWriter::write(unsigned bits)
{
	// check with _encoder for tile, then place it in template according to mapping
	// mapping will track current index/location?
	if (done())
		return false;

	CellPosition::coordinate xy = _position.next();
	cv::Mat cell = _encoder.encode(bits);
	paste(_image, cell, xy.first, xy.second);
	return true;
}

bool CimbWriter::done() const
{
	return _position.done();
}

cv::Mat CimbWriter::image() const
{
	return _image;
}

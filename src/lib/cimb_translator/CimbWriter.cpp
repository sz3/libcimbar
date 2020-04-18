#include "CimbWriter.h"

#include "CimbAssets.h"
#include "serialize/format.h"
#include <string>
using std::string;

namespace {
	cv::Mat getAnchor(bool dark)
	{
		string name = dark? "anchor-dark" : "anchor-light";
		return CimbAssets::load_img(fmt::format("bitmap/{}.png", name));
	}

	void paste(cv::Mat& canvas, const cv::Mat& img, int x, int y)
	{
		img.copyTo(canvas(cv::Rect(x, y, img.cols, img.rows)));
	}
}

CimbWriter::CimbWriter(bool dark, unsigned size)
    : _position(9, 112, 8)  // spacing, dimensions, offset... these will need to go in a config object... the stupid project root can go in there too
    , _encoder()
{
	cv::Scalar bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
	_image = cv::Mat(size, size, CV_8UC3, bgcolor);

	cv::Mat anchor = getAnchor(dark);
	paste(_image, anchor, 0, 0);
	paste(_image, anchor, 0, size - anchor.cols);
	paste(_image, anchor, size - anchor.rows, 0);
	paste(_image, anchor, size - anchor.rows, size - anchor.cols);
}

bool CimbWriter::write(unsigned bits)
{
	// check with _encoder for tile, then place it in template according to mapping
	// mapping will track current index/location?
	if (_position.done())
		return false;

	CellPosition::coordinate xy = _position.next();
	cv::Mat cell = _encoder.encode(bits);
	paste(_image, cell, xy.first, xy.second);
	return true;
}

bool CimbWriter::save(std::string filename) const
{
	return cv::imwrite(filename, _image);
}

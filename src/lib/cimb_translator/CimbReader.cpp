/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbReader.h"

#include "CellDrift.h"
#include "Config.h"

#include "bit_file/bitmatrix.h"
#include <opencv2/opencv.hpp>

using namespace cimbar;

namespace {
	cv::Mat kernel()
	{
		static const cv::Mat k = (cv::Mat_<float>(3,3) <<  -0, -1, -0, -1, 4.5, -1, -0, -1, -0);
		return k;
	}

	template <typename MAT>
	void sharpenSymbolGrid(const MAT& img, cv::Mat& out)
	{
		cv::filter2D(img, out, -1, kernel());
	}

	template <typename MAT>
	bitbuffer preprocessSymbolGrid(const MAT& img, bool needs_sharpen)
	{
		int blockSize = 3; // default: no preprocessing

		cv::Mat symbols;
		if (needs_sharpen)
		{
			sharpenSymbolGrid(img, symbols);
			cv::cvtColor(symbols, symbols, cv::COLOR_BGR2GRAY);
			blockSize = 7;
		}
		else
			cv::cvtColor(img, symbols, cv::COLOR_BGR2GRAY);

		cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, blockSize, 0);

		bitbuffer bb(1024*128);
		bitmatrix::mat_to_bitbuffer(symbols, bb.get_writer());
		return bb;
	}
}

CimbReader::CimbReader(const cv::Mat& img, const CimbDecoder& decoder, bool needs_sharpen)
    : _image(img)
    , _cellSize(Config::cell_size() + 2)
    , _positions(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _decoder(decoder)
    , _good(_image.cols >= 1024 and _image.rows >= 1024)
{
	_grayscale = preprocessSymbolGrid(img, needs_sharpen);
}

CimbReader::CimbReader(const cv::UMat& img, const CimbDecoder& decoder, bool needs_sharpen)
    : CimbReader(img.getMat(cv::ACCESS_READ), decoder, needs_sharpen)
{
}

unsigned CimbReader::read(unsigned& bits)
{
	if (_positions.done())
		return 0;

	// need coordinate, index, and drift from next position
	auto [i, xy, drift] = _positions.next();
	int x = xy.first + drift.x();
	int y = xy.second + drift.y();
	bitmatrix cell(_grayscale, _image.cols, _image.rows, x-1, y-1);
	Cell color_cell(_image, x-1, y-1, _cellSize, _cellSize);

	unsigned drift_offset = 0;
	unsigned error_distance;
	bits = _decoder.decode(cell, color_cell, drift_offset, error_distance);

	std::pair<int, int> best_drift = CellDrift::driftPairs[drift_offset];
	drift.updateDrift(best_drift.first, best_drift.second);
	_positions.update(i, drift, error_distance);
	return i;
}

bool CimbReader::done() const
{
	return !_good or _positions.done();
}

unsigned CimbReader::num_reads() const
{
	return _positions.size();
}

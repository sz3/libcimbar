/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbReader.h"

#include "CellDrift.h"
#include "Common.h"
#include "Config.h"

#include "bit_file/bitmatrix.h"
#include "chromatic_adaptation/color_correction.h"
#include "chromatic_adaptation/von_kries.h"
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
			cv::cvtColor(symbols, symbols, cv::COLOR_RGB2GRAY);
			blockSize = 7;
		}
		else
			cv::cvtColor(img, symbols, cv::COLOR_RGB2GRAY);

		cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, blockSize, 0);

		bitbuffer bb(1024*128);
		bitmatrix::mat_to_bitbuffer(symbols, bb.get_writer());
		return bb;
	}

	void updateMaxColor(std::tuple<double, double, double>& max_color, const cv::Scalar& c)
	{
		std::get<0>(max_color) = std::max(std::get<0>(max_color), c[2]);
		std::get<1>(max_color) = std::max(std::get<1>(max_color), c[1]);
		std::get<2>(max_color) = std::max(std::get<2>(max_color), c[0]);
	}

	std::tuple<double, double, double> calculateWhite(const cv::Mat& img, bool dark)
	{
		std::tuple<double, double, double> bestColor({1, 1, 1});
		if (dark)
		{
			unsigned tl = Config::anchor_size() - 2;
			unsigned br = Config::image_size() - Config::anchor_size() - 2;
			std::array<std::pair<unsigned, unsigned>, 3> anchors = {{ {tl, tl}, {tl, br}, {br, tl} }};
			for (auto [x, y] : anchors)
			{
				cv::Rect crop(x, y, 4, 4);
				cv::Scalar avgColor = cv::mean(img(crop));
				updateMaxColor(bestColor, avgColor);
			}
		}
		else // light
		{
			unsigned tl = (Config::anchor_size() << 1) + 6;
			unsigned br = Config::image_size() - tl - 4;
			std::array<std::pair<unsigned, unsigned>, 4> anchors = {{ {0, tl}, {tl, 0}, {0, br}, {br, 0} }};
			for (auto [x, y] : anchors)
			{
				cv::Rect crop(x, y, 4, 4);
				cv::Scalar avgColor = cv::mean(img(crop));
				updateMaxColor(bestColor, avgColor);
			}
		}
		return bestColor;
	}

	bool updateColorCorrection(const cv::Mat& img, CimbDecoder& decoder)
	{
		std::tuple<double, double, double> white = calculateWhite(img, Config::dark());
		decoder.update_color_correction(von_kries::get_adaptation_matrix(white, {255, 255, 255}));
		return true;
	}
}

CimbReader::CimbReader(const cv::Mat& img, CimbDecoder& decoder, bool needs_sharpen, bool color_correction)
    : _image(img)
    , _cellSize(Config::cell_size() + 2)
    , _positions(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _decoder(decoder)
    , _good(_image.cols >= Config::image_size() and _image.rows >= Config::image_size())
{
	_grayscale = preprocessSymbolGrid(img, needs_sharpen);
	if (_good and color_correction)
		updateColorCorrection(_image, decoder);
}

CimbReader::CimbReader(const cv::UMat& img, CimbDecoder& decoder, bool needs_sharpen, bool color_correction)
    : CimbReader(img.getMat(cv::ACCESS_READ), decoder, needs_sharpen, color_correction)
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

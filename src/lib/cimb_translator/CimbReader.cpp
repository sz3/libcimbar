/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbReader.h"

#include "CellDrift.h"
#include "Config.h"

#include "bit_file/bitmatrix.h"
#include "chromatic_adaptation/adaptation_transform.h"
#include "chromatic_adaptation/color_correction.h"
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
		int blockSize = 7;

		cv::Mat symbols;
		cv::cvtColor(img, symbols, cv::COLOR_RGB2GRAY);
		if (needs_sharpen)
			sharpenSymbolGrid(symbols, symbols); // we used to change blockSize for this case -- may one day be a useful trick again?
		cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, blockSize, 0);

		bitbuffer bb(std::pow(Config::image_size(), 2) / 8);
		bitmatrix::mat_to_bitbuffer(symbols, bb.get_writer());
		return bb;
	}

	void updateMaxColor(std::tuple<float, float, float>& max_color, const cv::Scalar& c)
	{
		std::get<0>(max_color) = std::max(std::get<0>(max_color), static_cast<float>(c[0]));
		std::get<1>(max_color) = std::max(std::get<1>(max_color), static_cast<float>(c[1]));
		std::get<2>(max_color) = std::max(std::get<2>(max_color), static_cast<float>(c[2]));
	}

	std::tuple<float, float, float> calculateWhite(const cv::Mat& img, bool dark)
	{
		std::tuple<float, float, float> bestColor({1, 1, 1});
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
		std::tuple<float, float, float> white = calculateWhite(img, Config::dark());
		decoder.update_color_correction(color_correction::get_adaptation_matrix<adaptation_transform::von_kries>(white, {255.0, 255.0, 255.0}));
		return true;
	}
}

CimbReader::CimbReader(const cv::Mat& img, CimbDecoder& decoder, bool needs_sharpen, bool color_correction)
	: _image(img)
	, _cellSize(Config::cell_size() + 2)
	, _positions(Config::cell_spacing(), Config::cells_per_col(), Config::cell_offset(), Config::corner_padding())
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

unsigned CimbReader::read_color(const PositionData& pos)
{
	Cell color_cell(_image, pos.x, pos.y, Config::cell_size(), Config::cell_size());
	return _decoder.decode_color(color_cell);
}

unsigned CimbReader::read(PositionData& pos)
{
	if (done())
		return 0;

	// need coordinate, index, and drift from next position
	auto [i, xy, drift, cooldown] = _positions.next();
	int x = xy.first + drift.x();
	int y = xy.second + drift.y();
	bitmatrix cell(_grayscale, _image.cols, _image.rows, x-1, y-1);

	unsigned drift_offset = 0;
	unsigned error_distance;
	unsigned bits = _decoder.decode_symbol(cell, drift_offset, error_distance, cooldown);

	std::pair<int, int> best_drift = CellDrift::driftPairs[drift_offset];
	drift.updateDrift(best_drift.first, best_drift.second);
	_positions.update(i, drift, error_distance, CellDrift::calculate_cooldown(cooldown, drift_offset));

	pos.i = i;
	pos.x = x + best_drift.first;
	pos.y = y + best_drift.second;
	return bits;
}

bool CimbReader::done() const
{
	return !_good or _positions.done();
}

unsigned CimbReader::num_reads() const
{
	return _positions.size();
}

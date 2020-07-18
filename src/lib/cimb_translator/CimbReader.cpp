#include "CimbReader.h"

#include "CellDrift.h"
#include "Config.h"
#include <opencv2/opencv.hpp>

using namespace cimbar;

namespace {
	cv::Mat kernel()
	{
		static const cv::Mat k = (cv::Mat_<float>(3,3) <<  -1, -1, -1, -1, 8.5, -1, -1, -1, -1);
		return k;
	}

	template <typename MAT>
	void sharpenSymbolGrid(const MAT& img, cv::Mat& out)
	{
		cv::filter2D(img, out, -1, kernel());
	}

	template <typename MAT>
	cv::Mat preprocessSymbolGrid(const MAT& img, bool needs_sharpen, bool needs_threshold)
	{
		cv::Mat symbols;
		if (needs_sharpen)
		{
			sharpenSymbolGrid(img, symbols);
			cv::cvtColor(symbols, symbols, cv::COLOR_BGR2GRAY);
		}
		else
			cv::cvtColor(img, symbols, cv::COLOR_BGR2GRAY);

		if (needs_threshold)
			cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 5, 0);
		return symbols;
	}
}

CimbReader::CimbReader(const cv::Mat& img, const CimbDecoder& decoder, bool needs_sharpen)
    : _image(img)
    , _cellSize(Config::cell_size() + 2)
    , _positions(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _decoder(decoder)
{
	_grayscale = preprocessSymbolGrid(img, needs_sharpen, decoder.expects_binary_threshold());
}

CimbReader::CimbReader(const cv::UMat& img, const CimbDecoder& decoder, bool needs_sharpen)
    : _image(img.getMat(cv::ACCESS_READ))
    , _cellSize(Config::cell_size() + 2)
    , _positions(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _decoder(decoder)
{
	_grayscale = preprocessSymbolGrid(img, needs_sharpen, decoder.expects_binary_threshold());
}

unsigned CimbReader::read(unsigned& bits)
{
	if (_positions.done())
		return 0;

	// need coordinate, index, and drift from next position
	auto [i, xy, drift] = _positions.next();
	int x = xy.first + drift.x();
	int y = xy.second + drift.y();
	cv::Rect crop(x-1, y-1, _cellSize, _cellSize);
	cv::Mat cell = _grayscale(crop);
	cv::Mat color_cell = _image(crop);

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
	return _positions.done();
}

unsigned CimbReader::num_reads() const
{
	return _positions.size();
}

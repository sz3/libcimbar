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

	void preprocessSymbolGrid(cv::Mat& img)
	{
		cv::filter2D(img, img, -1, kernel());
	}

	cv::UMat umat_kernel()
	{
		cv::Mat kern = kernel();
		cv::UMat k;
		kern.copyTo(k);
		return k;
	}

	void preprocessSymbolGrid(cv::UMat& img)
	{
		static const cv::UMat uk = umat_kernel();
		cv::filter2D(img, img, -1, uk);
	}
}

CimbReader::CimbReader(const cv::Mat& img, const CimbDecoder& decoder, bool should_preprocess)
    : _image(img)
    , _cellSize(Config::cell_size() + 2)
    , _positions(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _decoder(decoder)
{
	if (should_preprocess)
	{
		_grayscale = img.clone();
		preprocessSymbolGrid(_grayscale);
		cv::cvtColor(_grayscale, _grayscale, cv::COLOR_BGR2GRAY);
	}
	else
		cv::cvtColor(_image, _grayscale, cv::COLOR_BGR2GRAY);
}

CimbReader::CimbReader(const cv::UMat& img, const CimbDecoder& decoder, bool should_preprocess)
    : _image(img.getMat(cv::ACCESS_READ))
    , _cellSize(Config::cell_size() + 2)
    , _positions(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _decoder(decoder)
{
	if (should_preprocess)
	{
		cv::UMat temp = img.clone();
		preprocessSymbolGrid(temp);
		cv::cvtColor(temp, temp, cv::COLOR_BGR2GRAY);
		_grayscale = temp.getMat(cv::ACCESS_READ).clone();
	}
	else
		cv::cvtColor(_image, _grayscale, cv::COLOR_BGR2GRAY);
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
	return _positions.count();
}

#include "CimbReader.h"

#include "CellDrift.h"
#include "Config.h"
#include <opencv2/opencv.hpp>

using namespace cimbar;

CimbReader::CimbReader(const cv::Mat& img, const CimbDecoder& decoder)
    : _image(img)
    , _cellSize(Config::cell_size() + 2)
    , _position(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _drift()
    , _decoder(decoder)
{
	cv::cvtColor(_image, _grayscale, cv::COLOR_BGR2GRAY);
}

unsigned CimbReader::read()
{
	if (_position.done())
		return 0;

	CellPosition::coordinate xy = _position.next();
	int x = xy.first + _drift.x();
	int y = xy.second + _drift.y();
	cv::Rect crop(x-1, y-1, _cellSize, _cellSize);
	cv::Mat cell = _grayscale(crop);
	cv::Mat color_cell = _image(crop);

	unsigned drift_offset = 0;
	unsigned bits = _decoder.decode(cell, color_cell, drift_offset);

	std::pair<int, int> best_drift = CellDrift::driftPairs[drift_offset];
	_drift.updateDrift(best_drift.first, best_drift.second);
	return bits;
}

bool CimbReader::done() const
{
	return _position.done();
}

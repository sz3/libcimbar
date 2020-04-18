#include "CimbReader.h"

#include "CellDrift.h"
#include "Config.h"

using namespace cimbar;

CimbReader::CimbReader(std::string filename)
    : CimbReader(cv::imread(filename))
{}

CimbReader::CimbReader(const cv::Mat& img)
    : _image(img)
    , _cellSize(Config::cell_size())
    , _position(Config::cell_spacing(), Config::num_cells(), Config::cell_size(), Config::corner_padding())
    , _drift()
    , _decoder(Config::symbol_bits(), Config::color_bits())
{
}

unsigned CimbReader::read()
{
	if (_position.done())
		return 0;

	CellPosition::coordinate xy = _position.next();
	xy.first += _drift.x();
	xy.second += _drift.y();

	unsigned best_bits = 0;
	unsigned best_distance = 10000;
	std::pair<int, int> best_drift({0, 0});
	for (std::pair<int, int> trial : _drift.driftPairs())
	{
		int x = xy.first + trial.first;
		int y = xy.second + trial.second;
		cv::Rect crop(x, y, _cellSize, _cellSize);
		cv::Mat cell = _image(crop);

		unsigned distance;
		unsigned bits = _decoder.decode(cell, distance);
		if (distance < best_distance)
		{
			best_bits = bits;
			best_distance = distance;
			best_drift = trial;
			if (best_distance < 8)
				break;
		}
	}
	_drift.updateDrift(best_drift.first, best_drift.second);
	return best_bits;
}

bool CimbReader::done() const
{
	return _position.done();
}

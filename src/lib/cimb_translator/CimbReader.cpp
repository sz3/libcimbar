#include "CimbReader.h"

#include "CellDrift.h"

const unsigned cellSize = 8; // goes into settings?

CimbReader::CimbReader(std::string filename)
    : _position(9, 112, 8)  // standard complaints about how this should be a config apply
    , _drift()
    , _decoder(4, 2)
{
	load(filename);
}

bool CimbReader::load(std::string filename)
{
	_image = cv::imread(filename);
	return true;
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
		cv::Rect crop(x, y, cellSize, cellSize);
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

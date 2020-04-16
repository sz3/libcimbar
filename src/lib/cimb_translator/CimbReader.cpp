#include "CimbReader.h"

#include "CellDrift.h"

const unsigned cellSize = 8; // goes into settings?

CimbReader::CimbReader(std::string filename)
    : _position(9, 112, 8)  // standard complaints about how this should be a config apply
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

	CellDrift drift;
	CellPosition::coordinate xy = _position.next();
	xy.first += drift.x();
	xy.second += drift.y();

	cv::Rect crop(xy.first, xy.second, cellSize, cellSize);
	cv::Mat cell = _image(crop);
	return _decoder.decode(cell);
}

bool CimbReader::done() const
{
	return _position.done();
}

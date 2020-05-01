#include "CellDrift.h"
#include <vector>
#include <utility>

CellDrift::CellDrift(int limit)
    : _limit(limit)
{}

const std::vector<std::pair<int, int>> CellDrift::driftPairs = {
    {0, 0}, {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    //{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}
};

int CellDrift::x() const
{
	return _x;
}

int CellDrift::y() const
{
	return _y;
}

void CellDrift::updateDrift(int dx, int dy)
{
	_x += dx;
	_y += dy;
	if (_x > _limit)
		_x = _limit;
	else if (_x < (0-_limit))
		_x = 0-_limit;
	if (_y > _limit)
		_y = _limit;
	else if (_y < (0-_limit))
		_y = 0-_limit;
}

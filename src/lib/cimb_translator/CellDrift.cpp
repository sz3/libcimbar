#include "CellDrift.h"

CellDrift::CellDrift(int limit)
    : _limit(limit)
{}

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
	_x = std::min(_x, _limit);
	_x = std::max(_x, 0-_limit);
	_y += dy;
	_y = std::min(_y, _limit);
	_y = std::max(_y, 0-_limit);
}

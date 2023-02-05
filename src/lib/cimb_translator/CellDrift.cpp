/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CellDrift.h"

namespace {
	const int _limit = 7; // max number of pixels we drift.
}

CellDrift::CellDrift(int x, int y)
	: _x(x)
	, _y(y)
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

// static
uint8_t CellDrift::calculate_cooldown(uint8_t previous, uint8_t idx)
{
	if (idx % 2 == 0)
		return 0xFF;
	// if idx % 2 == 1 and previous xor idx == 6
	// return 0xFF; // ?
	if (previous == 1 and idx == 7)
		return 0xFF;
	if (previous == 3 and idx == 5)
		return 0xFF;
	if (previous == 5 and idx == 3)
		return 0xFF;
	if (previous == 7 and idx == 1)
		return 0xFF;
	return idx;
}

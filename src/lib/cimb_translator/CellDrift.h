#pragma once

#include <vector>
#include <utility>

template <int _limit=2>
class CellDrift
{
public:
	CellDrift() {}

	const std::vector<std::pair<int, int>> driftPairs() const
	{
		static std::vector<std::pair<int, int>> drift = {
		    {0, 0}, {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
		};
		return drift;
	}

	int x() const
	{
		return _x;
	}

	int y() const
	{
		return _y;
	}

	void updateDrift(int dx, int dy)
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

protected:
	int _x = 0;
	int _y = 0;
};

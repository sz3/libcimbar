#pragma once

#include <array>
#include <utility>

class CellDrift
{
public:
	CellDrift(int limit=2);

	inline static const std::array<std::pair<int, int>, 9> driftPairs = {{
	    {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}
	}};

	int x() const;
	int y() const;

	void updateDrift(int dx, int dy);

protected:
	int _limit;
	int _x = 0;
	int _y = 0;
};

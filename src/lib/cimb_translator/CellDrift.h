/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <array>
#include <utility>

class CellDrift
{
public:
	CellDrift(int x=0, int y=0);

	inline static const std::array<std::pair<int, int>, 9> driftPairs = {{
	    {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}
	}};

	int x() const;
	int y() const;

	void updateDrift(int dx, int dy);

protected:
	int _x;
	int _y;
};

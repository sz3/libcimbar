/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Point.h"
#include <cmath>
#include <iostream>
#include <utility>

class Anchor
{
public:
	Anchor()
	    : Anchor(0, 0, 0, 0)
	{}

	Anchor(int x, int xmax, int y, int ymax)
	    : _x(x)
	    , _xmax(xmax)
	    , _y(y)
	    , _ymax(ymax)
	{}

	void merge(const Anchor& other)
	{
		_x = std::min(_x, other.x());
		_xmax = std::max(_xmax, other.xmax());
		_y = std::min(_y, other.y());
		_ymax = std::max(_ymax, other.ymax());
	}

	point<int> center() const
	{
		return {xavg(), yavg()};
	}

	int x() const
	{
		return _x;
	}

	int xmax() const
	{
		return _xmax;
	}

	int xavg() const
	{
		return (_x + _xmax) / 2;
	}

	int xrange() const
	{
		return ::abs(_x - _xmax) / 2;
	}

	int y() const
	{
		return _y;
	}

	int ymax() const
	{
		return _ymax;
	}

	int yavg() const
	{
		return (_y + _ymax) / 2;
	}

	int yrange() const
	{
		return ::abs(_y - _ymax) / 2;
	}

	unsigned long long size() const
	{
		return std::pow(_x - _xmax, 2) + std::pow(_y - _ymax, 2);
	}

	int max_range() const
	{
		return std::max(::abs(_x - _xmax), ::abs(_y - _ymax));
	}

	bool is_mergeable(const Anchor& rhs, int max_distance) const
	{
		if (::abs(xavg() - rhs.xavg()) > max_distance or ::abs(yavg() - rhs.yavg()) > max_distance)
			return false;

		int ratio = (rhs.max_range() * 10 / max_range());
		return ratio > 6 and ratio < 17; // rhs within 60% of our size
	}

	bool operator<(const Anchor& rhs) const
	{
		return xavg() + yavg() < rhs.xavg() + rhs.yavg();
	}

	friend std::ostream& operator<<(std::ostream& os, const Anchor& anchor);

protected:
	int _x;
	int _xmax;
	int _y;
	int _ymax;
};

inline std::ostream& operator<<(std::ostream& outstream, const Anchor& anchor)
{
	outstream << anchor.xavg() << "+-" << anchor.xrange() << "," << anchor.yavg() << "+-" << anchor.yrange();
	return outstream;
}

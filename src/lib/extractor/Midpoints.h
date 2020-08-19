/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Point.h"
#include <vector>

class Midpoints
{
public:
	Midpoints()
	{}

	Midpoints(const std::vector<point<double>>& points)
	    : _points(points)
	{
	}

	bool operator!() const
	{
		return _points.size() < 4;
	}

	const point<double>& top() const
	{
		return _points[0];
	}

	const point<double>& right() const
	{
		return _points[1];
	}

	const point<double>& bottom() const
	{
		return _points[2];
	}

	const point<double>& left() const
	{
		return _points[3];
	}

	const std::vector<point<double>>& points() const
	{
		return _points;
	}

protected:
	std::vector<point<double>> _points;
};

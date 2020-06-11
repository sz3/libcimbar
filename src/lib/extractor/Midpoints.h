#pragma once

#include "Point.h"
#include <vector>

class Midpoints
{
public:
	Midpoints(const std::vector<point>& points)
		: _points(points)
	{
	}

	bool operator!() const
	{
		return _points.size() < 4;
	}

	const point& top() const
	{
		return _points[0];
	}

	const point& bottom() const
	{
		return _points[1];
	}

	const point& left() const
	{
		return _points[2];
	}

	const point& right() const
	{
		return _points[3];
	}

protected:
	std::vector<point> _points;
};

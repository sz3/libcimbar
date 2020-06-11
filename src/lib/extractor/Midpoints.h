#pragma once

#include "Point.h"
#include <vector>

class Midpoints
{
public:
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

	const point<double>& bottom() const
	{
		return _points[1];
	}

	const point<double>& left() const
	{
		return _points[2];
	}

	const point<double>& right() const
	{
		return _points[3];
	}

protected:
	std::vector<point<double>> _points;
};

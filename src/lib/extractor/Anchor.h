#pragma once

#include <iostream>

class Anchor
{
public:
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
		return (_x + _xmax) >> 1;
	}

	int xrange() const
	{
		return ::abs(_x - _xmax) >> 1;
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
		return (_y + _ymax) >> 1;
	}

	int yrange() const
	{
		return ::abs(_y - _ymax) >> 1;
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
	outstream << "x=" << anchor.x() << "-" << anchor.xmax() << ",y=" << anchor.y() << "-" << anchor.ymax();
	return outstream;
}

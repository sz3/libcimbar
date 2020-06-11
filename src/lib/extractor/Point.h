#pragma once

#include <climits>
#include <cmath>
#include <utility>

class point : private std::pair<int, int>
{
protected:
	typedef std::pair<int, int> base_pair;

public:
	static inline const point NONE()
	{
		return {INT_MIN, INT_MIN};
	}

	using base_pair::pair;

	int x() const
	{
		return this->first;
	}

	int y() const
	{
		return this->second;
	}

	bool operator==(const point& rhs) const
	{
		return this->first == rhs.first and this->second == rhs.second;
	}


	point operator+(const point& rhs) const
	{
		return {x() + rhs.x(), y() + rhs.y()};
	}

	point operator-(const point& rhs) const
	{
		return {x() - rhs.x(), y() - rhs.y()};
	}

	point operator/(double div) const
	{
		return {x() / div, y() / div};
	}

	point operator*(double scalar) const
	{
		return {x() * scalar, y() * scalar};
	}

	point& operator+=(const point& rhs)
	{
		this->first += rhs.first;
		this->second += rhs.second;
		return *this;
	}

	int distance(const point& rhs) const
	{
		return std::pow(rhs.x() - x(), 2) + std::pow(rhs.y() - y(), 2);
	}
};

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <climits>
#include <cmath>
#include <ostream>
#include <sstream>
#include <utility>

template <typename V>
class point : private std::pair<V, V>
{
protected:
	typedef std::pair<V, V> base_pair;

public:
	using base_pair::base_pair;
	static inline const point NONE();

	V x() const
	{
		return this->first;
	}

	V y() const
	{
		return this->second;
	}

	operator bool() const
	{
		return *this != NONE();
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

	point operator+(V scalar) const
	{
		return {x() + scalar, y() + scalar};
	}

	point operator-(V scalar) const
	{
		return {x() - scalar, y() - scalar};
	}

	template <typename T>
	point operator/(T div) const
	{
		return {x() / div, y() / div};
	}

	template <typename T>
	point operator*(T scalar) const
	{
		return {x() * scalar, y() * scalar};
	}

	point& operator+=(const point& rhs)
	{
		this->first += rhs.first;
		this->second += rhs.second;
		return *this;
	}

	V dot(const point& rhs) const
	{
		return (rhs.x() * x()) + (rhs.y() * y());
	}

	V squared_distance(const point& rhs) const
	{
		return std::pow(rhs.x() - x(), 2) + std::pow(rhs.y() - y(), 2);
	}

	point<double> to_float() const
	{
		return point<double>(x(), y());
	}

	point<int> to_int() const
	{
		return point<int>(x(), y());
	}

	std::string str() const
	{
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}
};

template <>
inline const point<int> point<int>::NONE()
{
	return {INT_MIN, INT_MIN};
}

template <>
inline const point<double> point<double>::NONE()
{
	return {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
}

template <typename V>
inline std::ostream& operator<<(std::ostream& outstream, const point<V>& p)
{
	if (p == point<V>::NONE())
		outstream << "NONE";
	else
		outstream << p.x() << "," << p.y();
	return outstream;
}


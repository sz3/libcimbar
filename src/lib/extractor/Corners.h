#pragma once

#include <opencv2/opencv.hpp>
#include <tuple>
using point = std::tuple<int, int>;

class Corners
{
public:
	Corners(point top_left, point top_right, point bottom_left, point bottom_right)
	    : _top_left(top_left)
	    , _top_right(top_right)
	    , _bottom_left(bottom_left)
	    , _bottom_right(bottom_right)
	{
	}

	const point& top_left() const
	{
		return _top_left;
	}

	const point& top_right() const
	{
		return _top_right;
	}

	const point& bottom_right() const
	{
		return _bottom_right;
	}

	const point& bottom_left() const
	{
		return _bottom_left;
	}

	std::vector<cv::Point2f> all() const
	{
		std::vector<cv::Point2f> points;
		points.push_back(cv::Point2f(std::get<0>(_top_left), std::get<1>(_top_left)));
		points.push_back(cv::Point2f(std::get<0>(_top_right), std::get<1>(_top_right)));
		points.push_back(cv::Point2f(std::get<0>(_bottom_left), std::get<1>(_bottom_left)));
		points.push_back(cv::Point2f(std::get<0>(_bottom_right), std::get<1>(_bottom_right)));
		return points;
	}

	bool is_granular_scale(unsigned min_size) const
	{
		// if any of our edges are < min_size, return false -- this means we'll be upscaling when we run a deskew.
		return (
		        check_scaling(_top_left, _top_right, min_size) and
		        check_scaling(_top_right, _bottom_right, min_size) and
		        check_scaling(_bottom_right, _bottom_left, min_size) and
		        check_scaling(_bottom_left, _top_left, min_size)
		);
	}

protected:
	bool check_scaling(const point& a, const point& b, unsigned min_size) const
	{
		auto [x1, y1] = a;
		auto [x2, y2] = b;
		return abs(x1 - x2) > min_size or abs(y1 - y2) > min_size;
	}

protected:
	point _top_left;
	point _top_right;
	point _bottom_right;
	point _bottom_left;
};

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

protected:
	point _top_left;
	point _top_right;
	point _bottom_right;
	point _bottom_left;
};

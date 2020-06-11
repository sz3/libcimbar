#pragma once

#include "Point.h"
#include <opencv2/opencv.hpp>
#include <tuple>

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
		points.push_back(cv::Point2f(_top_left.x(), _top_left.y()));
		points.push_back(cv::Point2f(_top_right.x(), _top_right.y()));
		points.push_back(cv::Point2f(_bottom_left.x(), _bottom_left.y()));
		points.push_back(cv::Point2f(_bottom_right.x(), _bottom_right.y()));
		return points;
	}

protected:
	point _top_left;
	point _top_right;
	point _bottom_right;
	point _bottom_left;
};

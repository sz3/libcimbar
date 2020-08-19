/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

class DistortionParameters
{
public:
	cv::Mat camera;
	cv::Mat distortion;

public:
	DistortionParameters()
	    : camera()
	    , distortion()
	{}

	DistortionParameters(const cv::Mat& camera, const cv::Mat& distortion)
	    : camera(camera)
	    , distortion(distortion)
	{}

	operator bool() const
	{
		return camera.cols > 0;
	}
};

#pragma once

#include "DistortionParameters.h"
#include <opencv2/opencv.hpp>

template <typename CAMERA_CALIBRATOR>
class Undistort
{
public:
	Undistort()
	    : _params()
	{}

	Undistort(const DistortionParameters& params)
	    : _params(params)
	{}

	static DistortionParameters get_distortion_parameters(const cv::Mat& img)
	{
		return CAMERA_CALIBRATOR().scan(img);
	}

	bool undistort(const cv::Mat& img, cv::Mat& out)
	{
		if (!_params)
		{
			_params = get_distortion_parameters(img);
			if (!_params)
				return false;
		}

		cv::undistort(img, out, _params.camera, _params.distortion);
		return true;
	}

	void reset_distortion_params()
	{
		_params = {};
	}

protected:
	DistortionParameters _params;
};

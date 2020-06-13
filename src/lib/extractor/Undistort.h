#pragma once

#include "DistortionParameters.h"
#include <opencv2/opencv.hpp>

template <typename CAMERA_CALIBRATOR>
class Undistort
{
public:
	Undistort() {}

	Undistort(int width, int height, const DistortionParameters& params)
	{
		set_distortion_params(width, height, params);
	}

	static DistortionParameters get_distortion_parameters(const cv::Mat& img)
	{
		return CAMERA_CALIBRATOR().scan(img);
	}

	bool undistort(const cv::Mat& img, cv::Mat& out)
	{
		if (!_params)
		{
			if ( !set_distortion_params(img.cols, img.rows, get_distortion_parameters(img)) )
				return false;
		}

		cv::remap(img, out, _map1, _map2, cv::INTER_LINEAR);
		return true;
	}

	bool set_distortion_params(int width, int height, const DistortionParameters& params)
	{
		if (!params)
			return false;

		_params = params;
		cv::initUndistortRectifyMap(_params.camera, _params.distortion, cv::Mat(), _params.camera, cv::Size(width, height), CV_32FC1, _map1, _map2);
		return true;
	}

	void reset_distortion_params()
	{
		_params = {};
		_map1.release();
		_map2.release();
	}

protected:
	DistortionParameters _params;
	cv::Mat _map1;
	cv::Mat _map2;
};

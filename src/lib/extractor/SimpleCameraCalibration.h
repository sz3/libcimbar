#pragma once

#include "DistortionParameters.h"
#include "Point.h"
#include <vector>

class Corners;
class Midpoints;

class SimpleCameraCalibration
{
public:
	SimpleCameraCalibration();

	DistortionParameters scan(const cv::Mat& img);

protected:
	double calculate_distortion_factor(const Corners& corners, const Midpoints& mps, const std::vector<point<int>>& edges);

protected:
	int _size;
	double _targetRatio;
};

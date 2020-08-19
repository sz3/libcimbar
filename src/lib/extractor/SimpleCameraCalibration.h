/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Corners.h"
#include "DistortionParameters.h"
#include "Midpoints.h"
#include "Point.h"
#include "Scanner.h"
#include <vector>

// implements CAMERA_CALIBRATOR
class SimpleCameraCalibration
{
public:
	SimpleCameraCalibration();

	template <typename MAT>
	DistortionParameters scan(const MAT& img);

protected:
	double calculate_distortion_factor(const Corners& corners, const Midpoints& mps, const std::vector<point<int>>& edges);

	static DistortionParameters naive_radial_undistort(int width, int height, double distortion_factor);

protected:
	int _size;
	double _targetRatio;
};

template <typename MAT>
inline DistortionParameters SimpleCameraCalibration::scan(const MAT& img)
{
	Scanner scanner(img);
	std::vector<Anchor> anchors = scanner.scan();
	if (anchors.size() < 4)
		return {};

	Corners corners(anchors);
	Midpoints mps;
	std::vector<point<int>> edges = scanner.scan_edges(corners, mps);
	if (edges.size() < 4)
		return {};

	double distortion_factor = calculate_distortion_factor(corners, mps, edges);
	return naive_radial_undistort(img.cols, img.rows, distortion_factor);
}

inline DistortionParameters SimpleCameraCalibration::naive_radial_undistort(int width, int height, double distortion_factor)
{
	// we're treating distortion_factor as being close to k1
	DistortionParameters dp;

	dp.camera = (cv::Mat1d(3, 3) << width/4, 0, width/2, 0, height/4, height/2, 0, 0, 1);
	dp.distortion = (cv::Mat1d(1, 4) << distortion_factor, 0, 0, 0);
	return dp;
}

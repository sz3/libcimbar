/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Corners.h"

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

class Deskewer
{
public:
	Deskewer(unsigned total_size=1024, unsigned anchor_size=30);
	int total_size() const;

	template <typename MAT>
	MAT deskew(const MAT& img, const Corners& corners);

	cv::Mat deskew(std::string img, const Corners& corners);
	bool save(const cv::Mat& img, std::string path);

protected:
	int _totalSize;
	int _anchorSize;
};

template <typename MAT>
inline MAT Deskewer::deskew(const MAT& img, const Corners& corners)
{
	std::vector<cv::Point2f> outputPoints;
	outputPoints.push_back(cv::Point2f(_anchorSize, _anchorSize));
	outputPoints.push_back(cv::Point2f(_totalSize - _anchorSize, _anchorSize));
	outputPoints.push_back(cv::Point2f(_anchorSize, _totalSize - _anchorSize));
	outputPoints.push_back(cv::Point2f(_totalSize - _anchorSize, _totalSize - _anchorSize));

	MAT output(_totalSize, _totalSize, img.type());
	cv::Mat transform = cv::getPerspectiveTransform(corners.all(), outputPoints);

	cv::warpPerspective(img, output, transform, output.size(), cv::INTER_LINEAR);
	return output;
}

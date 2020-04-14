#include "Deskewer.h"

#include <iostream>
using cv::Point2f;
using std::vector;

Deskewer::Deskewer()
{
}

cv::Mat Deskewer::deskew(std::string img, const Corners& corners)
{
	return deskew(cv::imread(img), corners);
}

cv::Mat Deskewer::deskew(cv::Mat img, const Corners& corners)
{
	// getPerspectiveTransform
	// warpPerspective

	vector<Point2f> outputPoints;
	outputPoints.push_back(Point2f(_anchorSize, _anchorSize));
	outputPoints.push_back(Point2f(_totalSize - _anchorSize, _anchorSize));
	outputPoints.push_back(Point2f(_anchorSize, _totalSize - _anchorSize));
	outputPoints.push_back(Point2f(_totalSize - _anchorSize, _totalSize - _anchorSize));

	cv::Mat transform = cv::getPerspectiveTransform(corners.all(), outputPoints);
	cv::Mat output(_totalSize, _totalSize, img.type());

	cv::warpPerspective(img, output, transform, output.size(), cv::INTER_LINEAR);
	return output;
}

bool Deskewer::save(const cv::Mat& img, std::string path)
{
	cv::imwrite(path, img);
}

#include "Deskewer.h"

#include <iostream>
using cv::Point2f;
using std::vector;

Deskewer::Deskewer()
{
}

bool Deskewer::deskew(std::string img, const Corners& corners)
{
	return deskew(cv::imread(img), corners);
}

bool Deskewer::deskew(cv::Mat img, const Corners& corners)
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
	std::cout << output.size() << std::endl;
	cv::imwrite("/tmp/rotated.jpg", output);
}

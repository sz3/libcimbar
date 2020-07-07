#include "Deskewer.h"

#include <iostream>

Deskewer::Deskewer(unsigned total_size, unsigned anchor_size)
    : _totalSize(total_size)
    , _anchorSize(anchor_size)
{
}

int Deskewer::total_size() const
{
	return _totalSize;
}

cv::Mat Deskewer::deskew(std::string img, const Corners& corners)
{
	return deskew(cv::imread(img), corners);
}

bool Deskewer::save(const cv::Mat& img, std::string path)
{
	return cv::imwrite(path, img);
}

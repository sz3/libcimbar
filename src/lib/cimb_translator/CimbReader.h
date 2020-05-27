#pragma once

#include "CellDrift.h"
#include "CellPosition.h"
#include "CimbDecoder.h"

#include <opencv2/opencv.hpp>
#include <string>

class CimbReader
{
public:
	CimbReader(const cv::Mat& img, const CimbDecoder& decoder);

	unsigned read();
	bool done() const;

protected:
	cv::Mat _image;
	cv::Mat _grayscale;
	unsigned _cellSize;
	CellPosition _position;
	CellDrift _drift;
	const CimbDecoder& _decoder;
};

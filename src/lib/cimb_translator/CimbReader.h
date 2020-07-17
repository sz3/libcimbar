#pragma once

#include "CimbDecoder.h"
#include "FloodDecodePositions.h"

#include <opencv2/opencv.hpp>
#include <string>

class CimbReader
{
public:
	CimbReader(const cv::Mat& img, const CimbDecoder& decoder, bool needs_sharpen=false);
	CimbReader(const cv::UMat& img, const CimbDecoder& decoder, bool needs_sharpen=false);

	unsigned read(unsigned& bits);
	bool done() const;

	unsigned num_reads() const;

protected:
	cv::Mat _image;
	cv::Mat _grayscale;
	unsigned _cellSize;
	FloodDecodePositions _positions;
	const CimbDecoder& _decoder;
};

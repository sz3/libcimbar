#pragma once

#include "CimbDecoder.h"
#include "LinearDecodePositions.h"

#include <opencv2/opencv.hpp>
#include <string>

class CimbReader
{
public:
	CimbReader(const cv::Mat& img, const CimbDecoder& decoder, bool should_preprocess=false);

	unsigned read(unsigned& bits);
	bool done() const;

	unsigned num_reads() const;

protected:
	cv::Mat _image;
	cv::Mat _grayscale;
	unsigned _cellSize;
	LinearDecodePositions _positions;
	const CimbDecoder& _decoder;
};

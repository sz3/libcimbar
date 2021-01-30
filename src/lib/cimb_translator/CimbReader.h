/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CimbDecoder.h"
#include "FloodDecodePositions.h"

#include "bit_file/bitbuffer.h"
#include <opencv2/opencv.hpp>
#include <string>

class CimbReader
{
public:
	CimbReader(const cv::Mat& img, CimbDecoder& decoder, bool needs_sharpen=false, bool color_correction=true);
	CimbReader(const cv::UMat& img, CimbDecoder& decoder, bool needs_sharpen=false, bool color_correction=true);

	unsigned read(unsigned& bits);
	bool done() const;

	unsigned num_reads() const;

protected:
	cv::Mat _image;
	bitbuffer _grayscale;

	unsigned _cellSize;
	FloodDecodePositions _positions;
	const CimbDecoder& _decoder;
	bool _good;
};

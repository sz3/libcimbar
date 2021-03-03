/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "util/loop_iterator.h"
#include <opencv2/opencv.hpp>

namespace cimbar {

class shaky_cam
{
public:
	static constexpr std::array<std::pair<int, int>, 4> SHAKE_POS = {{
	    {0, 0}, {-8, -8}, {0, 0}, {8, 8}
	}};

public:
	shaky_cam(unsigned img_size, unsigned w, unsigned h, bool dark)
	    : _shakycam(true)
	    , _shakePos(SHAKE_POS)
	{
		unsigned minFrameSize = img_size + 16;
		w = std::max(w, minFrameSize);
		h = std::max(h, minFrameSize);
		_bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
		_frame = cv::Mat(w, h, CV_8UC3, _bgcolor);
	}

	unsigned width() const
	{
		return _frame.cols;
	}

	unsigned height() const
	{
		return _frame.rows;
	}

	bool toggle()
	{
		return _shakycam = !_shakycam;
	}

	void reset()
	{
		_shakePos.reset();
	}

	void shake()
	{
		if (_shakycam)
			++_shakePos;
	}

	cv::Mat& draw(const cv::Mat& img)
	{
		_frame = _bgcolor;

		int offsetX = (_frame.cols - img.cols) >> 1;
		int offsetY = (_frame.rows - img.rows) >> 1;
		if (_shakycam)
		{
			_frame = _bgcolor;
			if (_shakePos)
			{
				offsetX += (*_shakePos).first;
				offsetY += (*_shakePos).second;
			}
		}

		img.copyTo(_frame(cv::Rect(offsetX, offsetY, img.cols, img.rows)));
		return _frame;
	}

protected:
	bool _shakycam;
	cv::Scalar _bgcolor;
	cv::Mat _frame;
	loop_iterator<decltype(SHAKE_POS)> _shakePos;
};

}

#pragma once

#include "window_interface.h"

class window_cvhighgui : public window_interface<window_cvhighgui>
{
public:
	window_cvhighgui() {}

	bool should_close() const
	{
		return cv::getWindowProperty("image", cv::WND_PROP_AUTOSIZE) < 0;
	}

	void show(const cv::Mat& img, unsigned delay)
	{
		cv::imshow("image", img);
		cv::waitKey(delay); // functions as the frame delay... you can hold down a key to make it go faster
	}

protected:
};

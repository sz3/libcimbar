/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "window_interface.h"

#include "shaky_cam.h"

namespace cimbar {

class window_cvhighgui : public window_interface<window_cvhighgui>
{
public:
	window_cvhighgui(unsigned width, unsigned height, std::string title)
	    : _cam(std::min(width, height), width, height, true)
	    , _title(title)
	{}

	bool is_good() const
	{
		return true;
	}

	bool should_close() const
	{
		return cv::getWindowProperty(_title, cv::WND_PROP_AUTOSIZE) < 0;
	}

	void rotate(unsigned i=1)
	{
	}

	void shake(unsigned i=1)
	{
		if (i > 0)
			_cam.shake();
	}

	void show(const cv::Mat& img, unsigned delay)
	{
		cv::imshow(_title, _cam.draw(img));
		cv::waitKey(delay); // functions as the frame delay... you can hold down a key to make it go faster
	}

protected:
	cimbar::shaky_cam _cam;
	std::string _title;
};

}

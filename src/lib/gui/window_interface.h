/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

namespace cimbar {

template <class T>
struct window_interface
{
	bool is_good() const
	{
		return static_cast<const T*>(this)->is_good();
	}

	bool should_close() const
	{
		return static_cast<const T*>(this)->should_close();
	}

	void rotate(unsigned i)
	{
		static_cast<T*>(this)->rotate(i);
	}

	void show(const cv::Mat& img, unsigned delay)
	{
		static_cast<T*>(this)->show(img, delay);
	}
};

}

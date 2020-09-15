#pragma once

#include <opencv2/opencv.hpp>

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

	void show(const cv::Mat& img, unsigned delay)
	{
		static_cast<T*>(this)->show(img, delay);
	}
};

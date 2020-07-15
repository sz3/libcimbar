#pragma once

#include <opencv2/opencv.hpp>
#include <tuple>

class Cell
{
public:
	Cell(const cv::Mat& img)
	    : _img(img)
	{}

	std::tuple<uchar,uchar,uchar> mean_rgb()
	{
		if (_img.channels() < 3)
			return std::tuple<uchar,uchar,uchar>(0, 0, 0);

		uint16_t blue = 0;
		uint16_t green = 0;
		uint16_t red = 0;
		uint16_t count = 0;

		int yend = _img.rows * _img.channels();
		for (int i = 0; i < _img.cols; ++i)
		{
			const uchar* p = _img.ptr<uchar>(i);
			for (int j = 0; j < yend; j+=_img.channels(), ++count)
			{
				blue += p[j];
				green += p[j+1];
				red += p[j+2];
			}
		}

		if (!count)
			return std::tuple<uchar,uchar,uchar>(0, 0, 0);

		return std::tuple<uchar,uchar,uchar>(red/count, green/count, blue/count);
	}

	uchar mean_grayscale()
	{
		if (_img.channels() > 1)
			return 0;

		uint16_t total = 0;
		uint16_t count = 0;

		for (int i = 0; i < _img.cols; ++i)
		{
			const uchar* p = _img.ptr<uchar>(i);
			for (int j = 0; j < _img.rows; ++j, ++count)
				total += p[j];
		}

		if (!count)
			return 0;
		return (uchar)(total/count);
	}

protected:
	const cv::Mat& _img;
};

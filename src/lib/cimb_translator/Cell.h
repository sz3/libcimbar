#pragma once

#include <tuple>
#include <opencv2/opencv.hpp>

class Cell
{
public:
	Cell(const cv::Mat& img)
		: Cell(img, 0, 0, img.cols, img.rows)
	{}

	Cell(const cv::Mat& img, int xstart, int ystart, int cols, int rows)
		: _img(img)
		, _xstart(xstart)
		, _ystart(ystart)
		, _cols(cols)
		, _rows(rows)
	{}

	std::tuple<uchar,uchar,uchar> mean_rgb()
	{
		if (_img.channels() < 3)
			return std::tuple<uchar,uchar,uchar>(0, 0, 0);

		int cols = _cols * _img.channels();
		int ystart = _ystart * _img.channels();

		unsigned blue = 0;
		unsigned green = 0;
		unsigned red = 0;
		unsigned count = 0;

		for (int i = _xstart; i < _rows; ++i)
		{
			const uchar* p = _img.ptr<uchar>(i);
			for (int j = ystart; j < cols; j+=_img.channels(), ++count)
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

		unsigned total = 0;
		unsigned count = 0;

		for (int i = _xstart; i < _rows; ++i)
		{
			const uchar* p = _img.ptr<uchar>(i);
			for (int j = _ystart; j < _cols; ++j, ++count)
				total += p[j];
		}

		if (!count)
			return 0;
		return (uchar)(total/count);
	}

protected:
	const cv::Mat& _img;
	int _xstart;
	int _ystart;
	int _cols;
	int _rows;
};

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

class color_correction
{
public:
	template <typename AT>
	static inline cv::Matx<double, 3, 3> get_adaptation_matrix(const std::tuple<double, double, double>& actual, const std::tuple<double, double, double>& desired)
	{
		AT transform;
		cv::Matx<double, 3, 1> src(std::get<0>(actual), std::get<1>(actual), std::get<2>(actual));
		cv::Matx<double, 3, 1> dst(std::get<0>(desired), std::get<1>(desired), std::get<2>(desired));

		cv::Matx<double, 3, 1> m1 = transform() * src;
		cv::Matx<double, 3, 1> m2 = transform() * dst;

		cv::Matx<double, 3, 3> d = cv::Matx<double, 3, 3>::diag(m2.div(m1));
		return transform().inv() * d * transform();
	}

public:
	color_correction()
	    : _active(false)
	{
	}

	color_correction(cv::Matx<double, 3, 3>&& m)
	    : _m(m)
	    , _active(true)
	{
	}

	void update(cv::Matx<double, 3, 3>&& m)
	{
		_m = m;
		_active = true;
	}

	bool active() const
	{
		return _active;
	}

	std::tuple<double, double, double> transform(double r, double g, double b) const
	{
		cv::Matx<double, 3, 1> temp = _m * cv::Matx<double, 3, 1>(r, g, b);
		return {temp(0), temp(1), temp(2)};
	}

protected:
	cv::Matx<double, 3, 3> _m;
	bool _active;
};

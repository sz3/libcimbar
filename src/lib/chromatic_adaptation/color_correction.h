/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

// transforms are in adaptation_transform.h
// http://brucelindbloom.com/Eqn_ChromAdapt.html

class color_correction
{
public:
	template <typename AT>
	static inline cv::Matx<float, 3, 3> get_adaptation_matrix(const std::tuple<float, float, float>& actual, const std::tuple<float, float, float>& desired)
	{
		AT transform;
		cv::Matx<float, 3, 1> src(std::get<0>(actual), std::get<1>(actual), std::get<2>(actual));
		cv::Matx<float, 3, 1> dst(std::get<0>(desired), std::get<1>(desired), std::get<2>(desired));

		cv::Matx<float, 3, 1> m1 = transform() * src;
		cv::Matx<float, 3, 1> m2 = transform() * dst;

		cv::Matx<float, 3, 3> d = cv::Matx<float, 3, 3>::diag(m2.div(m1));
		return transform().inv() * d * transform();
	}

	static inline cv::Matx<float, 3, 3> get_moore_penrose_lsm(const cv::Mat& actual, const cv::Mat& desired)
	{
		// inspired by the python colour-science package. It's not complicated,
		// but I didn't know that going in.
		// See also:
		// https://en.wikipedia.org/wiki/Moore-Penrose_inverse
		cv::Mat x, y, z;
		cv::transpose(desired, x);
		cv::transpose(actual, y);
		cv::invert(y, z, cv::DECOMP_SVD);

		y = x * z;
		return y;
	}

public:
	color_correction()
		: _active(false)
	{
	}

	color_correction(cv::Matx<float, 3, 3>&& m)
		: _m(m)
		, _active(true)
	{
	}

	void update(cv::Matx<float, 3, 3>&& m)
	{
		_m = m;
		_active = true;
	}

	bool active() const
	{
		return _active;
	}

	std::tuple<float, float, float> transform(float r, float g, float b) const
	{
		cv::Matx<float, 3, 1> temp = _m * cv::Matx<float, 3, 1>(r, g, b);
		return {temp(0), temp(1), temp(2)};
	}

	const cv::Matx<float, 3, 3> mat() const
	{
		return _m;
	}

protected:
	cv::Matx<float, 3, 3> _m;
	bool _active;
};

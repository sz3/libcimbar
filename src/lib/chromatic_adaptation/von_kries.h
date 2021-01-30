/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

// http://brucelindbloom.com/Eqn_ChromAdapt.html
// https://github.com/gtaylor/python-colormath

namespace von_kries
{
	inline cv::Matx<double, 3, 3> MA()
	{
		static cv::Matx<double, 3, 3> transform(
		    0.40024, 0.70760, -0.08081,
		    -0.22630, 1.16532, 0.04570,
		    0.00000, 0.00000, 0.91822
		);
		return transform;
	}

	inline cv::Matx<double, 3, 3> get_adaptation_matrix(const std::tuple<double, double, double>& actual, const std::tuple<double, double, double>& desired)
	{
		cv::Matx<double, 3, 1> src(std::get<0>(actual), std::get<1>(actual), std::get<2>(actual));
		cv::Matx<double, 3, 1> dst(std::get<0>(desired), std::get<1>(desired), std::get<2>(desired));

		cv::Matx<double, 3, 1> m1 = MA() * src;
		cv::Matx<double, 3, 1> m2 = MA() * dst;

		cv::Matx<double, 3, 3> d = cv::Matx<double, 3, 3>::diag(m2.div(m1));
		return MA().inv() * d * MA();
	}

}

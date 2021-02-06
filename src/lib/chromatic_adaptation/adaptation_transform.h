/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

namespace adaptation_transform
{
	struct bradford
	{
		cv::Matx<double, 3, 3> operator()() const
		{
			static cv::Matx<double, 3, 3> transform(
				0.8951000,  0.2664000, -0.1614000,
			   -0.7502000,  1.7135000,  0.0367000,
				0.0389000, -0.0685000,  1.0296000
			);
			return transform;
		}
	};

	struct von_kries
	{
		cv::Matx<double, 3, 3> operator()() const
		{
			static cv::Matx<double, 3, 3> transform(
				0.40024, 0.70760, -0.08081,
				-0.22630, 1.16532, 0.04570,
				0.00000, 0.00000, 0.91822
			);
			return transform;
		}
	};
}

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

namespace adaptation_transform
{
	struct bradford
	{
		const cv::Matx<float, 3, 3>& operator()() const
		{
			static cv::Matx<float, 3, 3> transform(
			     0.8951000,  0.2664000, -0.1614000,
			    -0.7502000,  1.7135000,  0.0367000,
			     0.0389000, -0.0685000,  1.0296000
			);
			return transform;
		}
	};

	struct von_kries
	{
		const cv::Matx<float, 3, 3>& operator()() const
		{
			static cv::Matx<float, 3, 3> transform(
			     0.4002400,  0.7076000, -0.0808100,
			    -0.2263000,  1.1653200,  0.0457000,
			     0.0000000,  0.0000000,  0.9182200
			);
			return transform;
		}
	};
}

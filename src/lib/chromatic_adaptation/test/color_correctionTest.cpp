/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "adaptation_transform.h"
#include "color_correction.h"

#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using std::string;

TEST_CASE( "color_correctionTest/testTransform", "[unit]" )
{
	cv::Matx<float, 3, 3> mat = color_correction::get_adaptation_matrix<adaptation_transform::von_kries>({192, 255, 255}, {255, 255, 255});

	{
		std::stringstream ss;
		cv::Ptr<cv::Formatter> fmtr = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
		fmtr->set32fPrecision(5);

		ss << fmtr->format(cv::Mat(mat));
		assertEquals( "[1.0656, 0.21092, -0.01324;\n"
					  " 0.023168, 0.98723, -0.0046781;\n"
					  " 0, 0, 1]", ss.str() );
	}

	std::tuple<float, float, float> c = color_correction(std::move(mat)).transform(180, 98, 255);
	assertAlmostEquals( 209.09822971, std::get<0>(c) );
	assertAlmostEquals( 99.72629027, std::get<1>(c) );
	assertAlmostEquals( 255, std::get<2>(c) );
}

TEST_CASE( "color_correctionTest/testComputeMoorePenrose", "[unit]" )
{
	cv::Mat actual = (cv::Mat_<float>(5,3) <<
				   0, 142.31060606, 0,
				   0, 148.75, 148.75,
				   148.75, 148.75, 0,
				   148.75, 0, 148.75,
				   255, 255, 255);

	cv::Mat desired = (cv::Mat_<float>(5,3) <<
				   0, 255, 0,
				   0, 255, 255,
				   255, 255, 0,
				   255, 0, 255,
				   255, 255, 255);

	cv::Matx<float, 3, 3> mat = color_correction::get_moore_penrose_lsm(actual, desired);
	{
		std::stringstream ss;
		cv::Ptr<cv::Formatter> fmtr = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
		fmtr->set32fPrecision(5);

		ss << fmtr->format(cv::Mat(mat));
		assertEquals( "[1.5223, -0.10024, -0.19198;\n"
					  " -0.20533, 1.6441, -0.20533;\n"
					  " -0.19198, -0.10024, 1.5223]", ss.str() );
	}
}

TEST_CASE( "color_correctionTest/testComputeMoorePenrose.2", "[unit]" )
{
	cv::Mat actual = (cv::Mat_<float>(5,3) <<
					  14.58901515, 115.74431818, 39.88320707,
					  19.34027778, 124.4375, 115.37152778,
					  140.70486111, 137.45833333, 65.50694444,
					  131.59722222, 41.22222222, 104.84027778,
					  171.625, 163.625, 158.875);

	cv::Mat desired = (cv::Mat_<float>(5,3) <<
				   0, 255, 0,
				   0, 255, 255,
				   255, 255, 0,
				   255, 0, 255,
				   255, 255, 255);

	cv::Matx<float, 3, 3> mat = color_correction::get_moore_penrose_lsm(actual, desired);
	{
		std::stringstream ss;
		cv::Ptr<cv::Formatter> fmtr = cv::Formatter::get(cv::Formatter::FMT_DEFAULT);
		fmtr->set32fPrecision(5);

		ss << fmtr->format(cv::Mat(mat));
		assertEquals( "[2.0261, -0.21691, -0.19806;\n"
					  " -0.43823, 2.4563, -0.417;\n"
					  " -0.5577, -1.1443, 3.4819]", ss.str() );
	}
}

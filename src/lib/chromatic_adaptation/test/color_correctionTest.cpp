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
		ss << mat;
		assertEquals( "[1.0655777, 0.2109226, -0.013239831;\n"
		              " 0.023168325, 0.98723376, -0.0046780901;\n"
		              " 0, 0, 1]", ss.str() );
	}

	std::tuple<float, float, float> c = color_correction(std::move(mat)).transform(180, 98, 255);
	assertAlmostEquals( 209.09822971, std::get<0>(c) );
	assertAlmostEquals( 99.72629027, std::get<1>(c) );
	assertAlmostEquals( 255, std::get<2>(c) );
}


/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "von_kries.h"

#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using std::string;

TEST_CASE( "von_kriesTest/testGetAdaptationMatrix", "[unit]" )
{
	cv::Mat mat = von_kries::get_adaptation_matrix({192, 255, 255}, {255, 255, 255});

	{
		std::stringstream ss;
		ss << mat;
		assertEquals( "[1.065577644398845, 0.2109225385610203, -0.01323982375544508;\n"
		              " 0.02316834486251532, 0.9872336916455101, -0.004678092483168828;\n"
		              " 0, 0, 1]", ss.str() );
	}

	std::tuple<double, double, double> c = von_kries::transform(mat, 180, 98, 255);
	assertAlmostEquals( 209.09822971, std::get<0>(c) );
	assertAlmostEquals( 99.72629027, std::get<1>(c) );
	assertAlmostEquals( 255, std::get<2>(c) );
}


#include "unittest.h"

#include "SimpleCameraCalibration.h"

#include "DistortionParameters.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
	std::string get_sample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}
}

TEST_CASE( "SimpleCameraCalibrationTest/testGetParams", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color-ecc40-fountain.jpg"));


	SimpleCameraCalibration scc;
	DistortionParameters dp = scc.scan(img);

	std::stringstream cam;
	cam << dp.camera;

	std::stringstream dis;
	dis << dp.distortion;

	assertEquals( "[240, 0, 480;\n"
	              " 0, 320, 640;\n"
	              " 0, 0, 1]", cam.str() );
	assertEquals( "[-0.002561135546020082, 0, 0, 0]", dis.str() );
}

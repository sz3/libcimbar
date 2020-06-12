#include "unittest.h"

#include "SimpleCameraCalibration.h"

#include "DistortionParameters.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "SimpleCameraCalibrationTest/testGetParams", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4color-ecc40-fountain.jpg"));


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

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
	cv::Mat img = cv::imread(TestCimbar::getSample("6bit/4_30_f0_627.jpg"));


	SimpleCameraCalibration scc;
	DistortionParameters dp = scc.scan(img);

	std::stringstream cam;
	cam << dp.camera;

	std::stringstream dis;
	dis << dp.distortion;

	assertEquals( "[320, 0, 640;\n"
	              " 0, 240, 480;\n"
	              " 0, 0, 1]", cam.str() );
	assertEquals( "[-0.001852885275781788, 0, 0, 0]", dis.str() );
}

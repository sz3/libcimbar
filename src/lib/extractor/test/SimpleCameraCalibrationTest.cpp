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
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));


	SimpleCameraCalibration scc;
	DistortionParameters dp = scc.scan(img);

	std::stringstream cam;
	cam << dp.camera;

	std::stringstream dis;
	dis << dp.distortion;

	assertEquals( "[270, 0, 540;\n"
	              " 0, 405, 810;\n"
	              " 0, 0, 1]", cam.str() );
	assertEquals( "[-0.002599077037483976, 0, 0, 0]", dis.str() );
}

#include "unittest.h"

#include "Extractor.h"
#include "Undistort.h"

#include "SimpleCameraCalibration.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "UndistortTest/testUndistort", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4color-ecc40-fountain.jpg"));
	cv::Mat out;

	Undistort<SimpleCameraCalibration> und;
	assertTrue( und.undistort(img, out) );

	cv::imwrite("/tmp/undistort.png", out);
	// standard image similarity testing questions apply
}

TEST_CASE( "UndistortTest/testUndistortAndExtract", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4color-ecc40-fountain.jpg"));
	cv::Mat out;

	Undistort<SimpleCameraCalibration> und;
	assertTrue( und.undistort(img, out) );

	Extractor ex;
	assertTrue( ex.extract(out, out) );

	cv::imwrite("/tmp/undistort+extract.png", out);
}

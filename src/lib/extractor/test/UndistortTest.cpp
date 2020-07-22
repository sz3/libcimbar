#include "unittest.h"

#include "Undistort.h"

#include "Extractor.h"
#include "SimpleCameraCalibration.h"
#include "image_hash/average_hash.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "UndistortTest/testUndistort", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	cv::Mat out;

	Undistort<SimpleCameraCalibration> und;
	assertTrue( und.undistort(img, out) );

	assertEquals( 0xa57ebeec300000, image_hash::average_hash(out) );
}

TEST_CASE( "UndistortTest/testUndistortAndExtract", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	cv::Mat out;

	Undistort<SimpleCameraCalibration> und;
	assertTrue( und.undistort(img, out) );

	Extractor ex;
	assertTrue( ex.extract(out, out) );

	assertEquals( 0x4dfdee6db91802, image_hash::average_hash(out) );
}

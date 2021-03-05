/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "gui/window_glfw.h"
#include "image_hash/average_hash.h"
#include <opencv2/opencv.hpp>

TEST_CASE( "window_glfwTest/testDefault", "[unit]" )
{
	cv::Mat sample = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");
	assertEquals( 1024, sample.cols ); // sanity check

	// create a window
	cimbar::window_glfw window(sample.cols, sample.rows, "test window");
	assertTrue( window.is_good() );

	window.show(sample, 0);

	// check state
	cv::Mat res = window.capture_gl_state();
	assertEquals( image_hash::average_hash(sample), image_hash::average_hash(res) );
}



/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "CimbWriter.h"
#include "image_hash/average_hash.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CimbWriterTest/testSimple", "[unit]" )
{
	CimbWriter cw(4, 2);

	while (1)
	{
		if (!cw.write(0))
			break;
	}

	cv::Mat img = cw.image();
	assertEquals(1024, img.cols);
	assertEquals(1024, img.rows);
	assertEquals( 0xeecc8800efce8c08, image_hash::average_hash(img) );
}

TEST_CASE( "CimbWriterTest/testCustomSize", "[unit]" )
{
	CimbWriter cw(4, 2, true, 1, 1040);

	while (1)
	{
		if (!cw.write(0))
			break;
	}

	cv::Mat img = cw.image();
	assertEquals(1040, img.cols);
	assertEquals(1040, img.rows);
	assertEquals( 0xab00ab02af0abfab, image_hash::average_hash(img) );
}

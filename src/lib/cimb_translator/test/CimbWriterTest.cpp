#include "unittest.h"

#include "CimbWriter.h"
#include "image_hash/average_hash.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CimbWriterTest/testSimple", "[unit]" )
{
	CimbWriter cw;

	while (1)
	{
		if (!cw.write(0))
			break;
	}

	cv::Mat img = cw.image();
	assertEquals( 0xeecc8800efce8c08, image_hash::average_hash(img) );
}

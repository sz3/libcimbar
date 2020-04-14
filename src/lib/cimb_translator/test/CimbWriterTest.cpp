#include "unittest.h"

#include "CimbWriter.h"

#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CimbWriterTest/testSimple", "[unit]" )
{
	CimbWriter cw(4);
	cv::Mat res = cw.encode(15);

	std::cout << res << std::endl;
}


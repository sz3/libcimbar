#include "unittest.h"

#include "CimbDecoder.h"

#include "CimbCommon.h"

#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
using std::string;

TEST_CASE( "CimbDecoderTest/testSimple", "[unit]" )
{
	CimbDecoder cd(4, 0);

	string tile_dir = CimbCommon::getTileDir(4);
	for (int i = 0; i < 16; ++i)
	{
		cv::Mat tile = CimbCommon::getTile(tile_dir, i);
		unsigned res = cd.decode(tile);
		assertEquals(i, res);
	}
}

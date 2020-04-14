#include "unittest.h"

#include "CimbWriter.h"

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

	cw.save("/tmp/howwedoin.png");
}

#include "unittest.h"

#include "Corners.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CornersTest/testSimple", "[unit]" )
{
	Corners corners({1, 1}, {20, 1}, {1, 30}, {40, 40});
	assertEquals(point({1, 1}), corners.top_left());
	assertEquals(point({20, 1}), corners.top_right());
	assertEquals(point({1, 30}), corners.bottom_left());
	assertEquals(point({40, 40}), corners.bottom_right());
}


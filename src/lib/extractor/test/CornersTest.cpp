#include "unittest.h"

#include "Corners.h"

#include "Geometry.h"
#include "Midpoints.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CornersTest/testSimple", "[unit]" )
{
	Corners corners({1, 1}, {20, 1}, {1, 30}, {40, 40});
	assertEquals(point<int>({1, 1}), corners.top_left());
	assertEquals(point<int>({20, 1}), corners.top_right());
	assertEquals(point<int>({1, 30}), corners.bottom_left());
	assertEquals(point<int>({40, 40}), corners.bottom_right());
}

TEST_CASE( "CornersTest/testMidpoints", "[unit]" )
{
	Corners corners({1, 1}, {20, 1}, {1, 30}, {40, 40});

	Midpoints mp = Geometry::calculate_midpoints(corners);
	assertEquals(point<int>({9, 1}), mp.top());
	assertEquals(point<int>({17, 34}), mp.bottom());
	assertEquals(point<int>({1, 11}), mp.left());
	assertEquals(point<int>({26, 13}), mp.right());
}


TEST_CASE( "CornersTest/testMidpoints.2", "[unit]" )
{
	Corners corners({69,192}, {840,156}, {148,890}, {809,884});

	Midpoints mp = Geometry::calculate_midpoints(corners);
	// top=(446.66222422083973, 174.36596618424093),
	// right=(823.2766663941112, 548.7286085511948),
	// bottom=(472.7464269951889, 887.0522260787124),
	// left=(111.50273051900028, 567.5304544590152)
	assertEquals(point<int>({446, 174}), mp.top());
	assertEquals(point<int>({472, 887}), mp.bottom());
	assertEquals(point<int>({111, 567}), mp.left());
	assertEquals(point<int>({823, 548}), mp.right());
}


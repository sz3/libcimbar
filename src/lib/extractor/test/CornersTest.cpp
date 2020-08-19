/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
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

	assertEquals("9.76387,1", mp.top().str());
	assertEquals("17.6324,34.2647", mp.bottom().str());
	assertEquals("1,11.4214", mp.left().str());
	assertEquals("26.5517,13.7759", mp.right().str());
}


TEST_CASE( "CornersTest/testMidpoints.2", "[unit]" )
{
	Corners corners({69,192}, {840,156}, {148,890}, {809,884});

	Midpoints mp = Geometry::calculate_midpoints(corners);
	// top=(446.66222422083973, 174.36596618424093),
	// right=(823.2766663941112, 548.7286085511948),
	// bottom=(472.7464269951889, 887.0522260787124),
	// left=(111.50273051900028, 567.5304544590152)
	assertEquals("446.662,174.366", mp.top().str());
	assertEquals("472.746,887.052", mp.bottom().str());
	assertEquals("111.503,567.53", mp.left().str());
	assertEquals("823.277,548.729", mp.right().str());
}


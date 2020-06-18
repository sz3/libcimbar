#include "unittest.h"

#include "Deskewer.h"
#include "image_hash/average_hash.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "DeskewerTest/testSimple", "[unit]" )
{
	Corners corners({169, 981}, {2409, 897}, {324, 3030}, {2343, 3066});
	Deskewer de;

	cv::Mat actual = de.deskew(TestCimbar::getSample("4color1.jpg"), corners);
	assertEquals(cv::Size(1024, 1024), actual.size());

	assertEquals( 0x434caa959f7e56c7, image_hash::average_hash(actual) );
}


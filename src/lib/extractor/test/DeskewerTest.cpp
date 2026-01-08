/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "DeskewerPlus.h"
#include "image_hash/average_hash.h"
#include <string>

TEST_CASE( "DeskewerTest/testSimple", "[unit]" )
{
	Corners corners({312, 519}, {323, 2586}, {2405, 461}, {2425, 2594});
	DeskewerPlus de(0, {1024, 1024}, 30);

	cv::Mat actual = de.deskew(TestCimbar::getSample("6bit/4_30_f0_big.jpg"), corners);
	assertEquals(cv::Size(1024, 1024), actual.size());

	assertEquals( 0x6e483730782fee5c, image_hash::average_hash(actual) );
}

TEST_CASE( "DeskewerTest/testPadded", "[unit]" )
{
	Corners corners({312, 519}, {323, 2586}, {2405, 461}, {2425, 2594});
	DeskewerPlus de(8, {1024, 1024}, 30);

	cv::Mat actual = de.deskew(TestCimbar::getSample("6bit/4_30_f0_big.jpg"), corners);
	assertEquals(cv::Size(1040, 1040), actual.size());

	cv::Rect crop(8, 8, 1024, 1024);
	cv::Mat innerGrid = actual(crop);

	assertEquals( 0x6e483730782fee5c, image_hash::average_hash(innerGrid) );
}



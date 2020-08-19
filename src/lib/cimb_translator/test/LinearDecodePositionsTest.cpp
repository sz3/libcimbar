/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "LinearDecodePositions.h"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "LinearDecodePositionsTest/testSimple", "[unit]" )
{
	LinearDecodePositions cells(9, 112, 8, 6);

	// test first coordinate. We'll just count the rest.
	assertFalse( cells.done() );
	auto [i, xy, drift] = cells.next();
	assertEquals(62, xy.first);
	assertEquals(8, xy.second);
	assertEquals(i, 0);

	unsigned count = 1;
	while (!cells.done())
	{
		auto [i, xy, drift] = cells.next();
		assertEquals(i, count);
		++count;
	}

	assertEquals(12400, count);
}

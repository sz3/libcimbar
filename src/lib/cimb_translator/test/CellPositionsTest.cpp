/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "CellPositions.h"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CellPositionsTest/testSimple", "[unit]" )
{
	CellPositions cells(cimbar::vec_xy{9, 9}, cimbar::vec_xy{112, 112}, 8, cimbar::vec_xy{6, 6});

	// test first coordinate. We'll just count the rest.
	assertFalse( cells.done() );
	CellPositions::coordinate xy = cells.next();
	assertEquals(62, xy.first);
	assertEquals(8, xy.second);

	unsigned count = 1;
	while (!cells.done())
	{
		cells.next();
		++count;
	}

	assertEquals(12400, count);
}

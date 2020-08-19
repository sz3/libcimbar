/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "CellPositions.h"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CellPositionsTest/testSimple", "[unit]" )
{
	CellPositions cells(9, 112, 8, 6);

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

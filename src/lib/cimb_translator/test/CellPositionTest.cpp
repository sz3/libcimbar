#include "unittest.h"

#include "CellPosition.h"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CellPositionTest/testSimple", "[unit]" )
{
	CellPosition cells(9, 112, 8, 6);

	// test first coordinate. We'll just count the rest.
	assertFalse( cells.done() );
	CellPosition::coordinate xy = cells.next();
	assertEquals(62, xy.first);
	assertEquals(9, xy.second);

	unsigned count = 1;
	while (!cells.done())
	{
		cells.next();
		++count;
	}

	assertEquals(12400, count);
}

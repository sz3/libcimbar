/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "AdjacentCellFinder.h"
#include "CellPositions.h"
#include "serialize/str_join.h"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "AdjacentCellFinderTest/testFirstSection", "[unit]" )
{
	// there are 600 cells in the first section
	CellPositions::positions_list positions = CellPositions::compute(9, 112, 8, 6, 0);
	AdjacentCellFinder finder(positions, 112, 6);

	// top row
	assertEquals( "1 -1 100 -1", turbo::str::join(finder.find(0)) );
	assertEquals( "2 0 101 -1", turbo::str::join(finder.find(1)) );
	assertEquals( "101 -1 200 0", turbo::str::join(finder.find(100)) );
	assertEquals( "-1 98 199 -1", turbo::str::join(finder.find(99)) );

	// bottom row of first section
	assertEquals( "501 -1 606 400", turbo::str::join(finder.find(500)) );
	assertEquals( "-1 598 705 499", turbo::str::join(finder.find(599)) );
}

TEST_CASE( "AdjacentCellFinderTest/testMiddle", "[unit]" )
{
	// 11200 cells in the middle
	CellPositions::positions_list positions = CellPositions::compute(9, 112, 8, 6, 0);
	AdjacentCellFinder finder(positions, 112, 6);

	// top row of "middle"
	assertEquals( "601 -1 712 -1", turbo::str::join(finder.find(600)) );
	assertEquals( "-1 710 823 -1", turbo::str::join(finder.find(711)) );
	assertEquals( "606 604 717 -1", turbo::str::join(finder.find(605)) );
	assertEquals( "607 605 718 500", turbo::str::join(finder.find(606)) );
	assertEquals( "707 705 818 -1", turbo::str::join(finder.find(706)) );
	assertEquals( "706 704 817 599", turbo::str::join(finder.find(705)) );

	// bottom row
	assertEquals( "11689 -1 -1 11576", turbo::str::join(finder.find(11688)) );
	assertEquals( "11694 11692 -1 11581", turbo::str::join(finder.find(11693)) );
	assertEquals( "11695 11693 11800 11582", turbo::str::join(finder.find(11694)) );
	assertEquals( "-1 11798 -1 11687", turbo::str::join(finder.find(11799)) );
}

TEST_CASE( "AdjacentCellFinderTest/testLastSection", "[unit]" )
{
	// 600 cells in the last section
	CellPositions::positions_list positions = CellPositions::compute(9, 112, 8, 6, 0);
	AdjacentCellFinder finder(positions, 112, 6);

	// bottom row of grid
	assertEquals( "12301 -1 -1 12200", turbo::str::join(finder.find(12300)) );
	assertEquals( "-1 12398 -1 12299", turbo::str::join(finder.find(12399)) );

	// top row of last section
	assertEquals( "11801 -1 11900 11694", turbo::str::join(finder.find(11800)) );
	assertEquals( "-1 11898 11999 11793", turbo::str::join(finder.find(11899)) );
}

#include "unittest.h"

#include "FloodDecodePositions.h"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "FloodDecodePositionsTest/testSimple", "[unit]" )
{
	const unsigned posCount = 12400;

	FloodDecodePositions cells(9, 112, 8, 6);
	std::set<unsigned> remainingPos;
	for (int i = 0; i < posCount; ++i)
		remainingPos.insert(i);

	// test a handful of coordinates. We'll just make sure we go through the rest in some order.
	assertFalse( cells.done() );

	unsigned i;
	CellPositions::coordinate xy;
	CellDrift drift;

	unsigned count = 0;
	// 0
	std::tie(i, xy, drift) = cells.next();
	assertEquals(i, 0);
	assertEquals(62, xy.first);
	assertEquals(8, xy.second);
	assertEquals(0, drift.x());
	assertEquals(0, drift.y());

	cells.update(i, drift, 1);
	remainingPos.erase(i);
	++count;

	// 1
	std::tie(i, xy, drift) = cells.next();
	assertEquals(i, 1);
	assertEquals(71, xy.first);
	assertEquals(8, xy.second);
	assertEquals(0, drift.x());
	assertEquals(0, drift.y());

	cells.update(i, drift, 2);
	remainingPos.erase(i);
	++count;

	while (!cells.done())
	{
		std::tie(i, xy, drift) = cells.next();
		cells.update(i, drift, 1);
		remainingPos.erase(i);
		++count;
	}

	assertEquals(posCount, count);
	assertEquals(0, remainingPos.size());
}

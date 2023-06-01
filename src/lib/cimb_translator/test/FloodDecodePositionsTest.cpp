/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "FloodDecodePositions.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>

TEST_CASE( "FloodDecodePositionsTest/testSimple", "[unit]" )
{
	const unsigned posCount = 12400;

	FloodDecodePositions cells(9, 112, 8, 6);
	std::set<unsigned> remainingPos;
	for (unsigned i = 0; i < posCount; ++i)
		remainingPos.insert(i);

	// test a handful of coordinates. We'll just make sure we go through the rest in some order.
	assertFalse( cells.done() );

	unsigned i;
	CellPositions::coordinate xy;
	CellDrift drift;
	uint8_t cooldown;

	// validate seed points
	unsigned count = 0;
	for (int c = 0; c < 8; ++c)
	{
		// 0
		std::tie(i, xy, drift, cooldown) = cells.next();
		assertEquals(0, drift.x());
		assertEquals(0, drift.y());

		if (i == 0)
		{
			assertEquals(62, xy.first);
			assertEquals(8, xy.second);
			drift.updateDrift(1, 1);
			cells.update(i, drift, 1, cooldown);
		}
		else if (i == 99)
		{
			assertEquals(953, xy.first);
			assertEquals(8, xy.second);
			cells.update(i, drift, 2, cooldown);
		}
		else if (i == 12300)
		{
			assertEquals(62, xy.first);
			assertEquals(1007, xy.second);
			cells.update(i, drift, 2, cooldown);
		}
		else if (i == 12399)
		{
			assertEquals(953, xy.first);
			assertEquals(1007, xy.second);
			cells.update(i, drift, 2, cooldown);
		}
		else if (i == 600)
		{
			assertEquals(8, xy.first);
			assertEquals(62, xy.second);
			cells.update(i, drift, 2, cooldown);
		}
		else if (i == 711)
		{
			assertEquals(1007, xy.first);
			assertEquals(62, xy.second);
			cells.update(i, drift, 2, cooldown);
		}
		else if (i == 11688)
		{
			assertEquals(8, xy.first);
			assertEquals(953, xy.second);
			cells.update(i, drift, 2, cooldown);
		}
		else if (i == 11799)
		{
			assertEquals(1007, xy.first);
			assertEquals(953, xy.second);
			cells.update(i, drift, 2, cooldown);
		}
		else
			FAIL("i ?= " << i);

		remainingPos.erase(i);
		++count;
	}

	// we did "update(..., 1) for index=1. So the next cells should be the ones adjacent to 0: 1, 100.
	for (int c = 0; c < 2; ++c)
	{
		std::tie(i, xy, drift, cooldown) = cells.next();
		if (i == 1)
		{
			assertEquals(71, xy.first);
			assertEquals(8, xy.second);
		}
		else if (i == 100)
		{
			assertEquals(62, xy.first);
			assertEquals(17, xy.second);
		}
		else
			FAIL("i ?= " << i);

		// we manually set drift to 1,1 -- it should be persisted
		assertEquals(1, drift.x());
		assertEquals(1, drift.y());

		cells.update(i, drift, 3, cooldown);
		remainingPos.erase(i);
		++count;
	}

	// as for the rest... we'll just make sure we hit them all
	while (!cells.done())
	{
		std::tie(i, xy, drift, cooldown) = cells.next();
		cells.update(i, drift, 1, cooldown);
		remainingPos.erase(i);
		++count;
	}

	assertEquals(posCount, count);
	assertEquals(0, remainingPos.size());
}

#include "unittest.h"

#include "ScanState.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "ScanStateTest/testScan", "[unit]" )
{
	// ratio is 1:1:4:1:1
	// state.process() will return -1 (NOOP) if there is nothing to do.
	// but it will return a positive integer (the size of the range) if we found a match.
	ScanState state;
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(true));
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(true));
	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(false));

	for (int i = 0; i < 3; ++i)
		assertEquals(ScanState::NOOP, state.process(true));
	for (int i = 0; i < 3; ++i)
		assertEquals(ScanState::NOOP, state.process(false));
	for (int i = 0; i < 12; ++i)
		assertEquals(ScanState::NOOP, state.process(true));
	for (int i = 0; i < 3; ++i)
		assertEquals(ScanState::NOOP, state.process(false));
	for (int i = 0; i < 3; ++i)
		assertEquals(ScanState::NOOP, state.process(true));

	// found one!
	assertEquals( 24, state.process(false) );

	assertEquals(ScanState::NOOP, state.process(false));
	assertEquals(ScanState::NOOP, state.process(false));
}

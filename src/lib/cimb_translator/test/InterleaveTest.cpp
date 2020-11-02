/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "Interleave.h"
#include "CellPositions.h"
#include "serialize/str_join.h"

#include <iostream>
#include <string>
#include <vector>
using std::string;


TEST_CASE( "InterleaveTest/testDefault", "[unit]" )
{
	std::vector<int> pos = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};

	std::vector<unsigned> indices = Interleave::interleave_indices(pos.size(), 2, 1);
	assertEquals( "0 2 4 6 8 1 3 5 7 9", turbo::str::join(indices) );

	std::vector<int> actual = Interleave::interleave(pos, 2, 1);
	assertEquals( "0 20 40 60 80 10 30 50 70 90", turbo::str::join(actual) );
}

TEST_CASE( "InterleaveTest/testPartitions", "[unit]" )
{
	std::vector<int> pos = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

	std::vector<unsigned> indices = Interleave::interleave_indices(pos.size(), 5, 2);
	assertEquals( "0 5 1 6 2 7 3 8 4 9 10 15 11 16 12 17 13 18 14 19", turbo::str::join(indices) );

	std::vector<int> actual = Interleave::interleave(pos, 5, 2);
	assertEquals( "0 5 1 6 2 7 3 8 4 9 10 15 11 16 12 17 13 18 14 19", turbo::str::join(actual) );
}

TEST_CASE( "InterleaveTest/testReverse", "[unit]" )
{
	std::vector<int> pos = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};

	std::vector<int> actual = Interleave::interleave(pos, 2, 1);
	assertEquals( "0 20 40 60 80 10 30 50 70 90", turbo::str::join(actual) );

	std::vector<unsigned> invert = Interleave::interleave_reverse(pos.size(), 2, 1);
	assertEquals( "0 5 1 6 2 7 3 8 4 9", turbo::str::join(invert) );

	std::vector<int> reconstruct(pos.size(), -1);
	for (unsigned i = 0; i < pos.size(); ++i)
		reconstruct[i] = actual[invert[i]];
	assertEquals( "0 10 20 30 40 50 60 70 80 90", turbo::str::join(reconstruct) );
}

TEST_CASE( "InterleaveTest/testCellPositions", "[unit]" )
{
	std::vector<unsigned> indices = Interleave::interleave_reverse(CellPositions::compute_linear(9, 112, 8, 6).size(), 155, 1);
	assertEquals( 12400, indices.size() );
	assertEquals( 0, indices[0] );
	assertEquals( 80, indices[1] );
	assertEquals( 160, indices[2] );
}

#include "unittest.h"

#include "Interleave.h"
#include "CellPosition.h"
#include "serialize/str_join.h"

#include <iostream>
#include <string>
#include <vector>
using std::string;


TEST_CASE( "InterleaveTest/testDefault", "[unit]" )
{
	std::vector<int> pos = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};

	std::vector<unsigned> indices = Interleave::interleave_indices(pos.size(), 2);
	assertEquals( "0 2 4 6 8 1 3 5 7 9", turbo::str::join(indices) );

	std::vector<int> actual = Interleave::interleave(pos, 2);
	assertEquals( "0 20 40 60 80 10 30 50 70 90", turbo::str::join(actual) );
}

TEST_CASE( "InterleaveTest/testReverse", "[unit]" )
{
	std::vector<int> pos = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};

	std::vector<int> actual = Interleave::interleave(pos, 2);
	assertEquals( "0 20 40 60 80 10 30 50 70 90", turbo::str::join(actual) );

	std::vector<unsigned> invert = Interleave::interleave_reverse(pos.size(), 2);
	assertEquals( "0 5 1 6 2 7 3 8 4 9", turbo::str::join(invert) );

	std::vector<int> reconstruct(pos.size(), -1);
	auto it = actual.begin();
	for (unsigned i = 0; i < pos.size(); ++i)
		reconstruct[i] = actual[invert[i]];
	assertEquals( "0 10 20 30 40 50 60 70 80 90", turbo::str::join(reconstruct) );
}

TEST_CASE( "InterleaveTest/testCellPositions", "[unit]" )
{
	std::vector<unsigned> indices = Interleave::interleave_reverse(CellPosition::compute_linear(9, 112, 8, 6).size(), 155);
	assertEquals( 12400, indices.size() );
	assertEquals( 0, indices[0] );
	assertEquals( 80, indices[1] );
	assertEquals( 160, indices[2] );
}

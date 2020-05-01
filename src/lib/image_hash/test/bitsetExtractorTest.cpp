#include "unittest.h"

#include "bitset_extractor.h"

#include <bitset>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;

TEST_CASE( "bitsetExtractorTest/testDefault", "[unit]" )
{
	std::bitset<32> bits(0x11d07e1f);
	bitset_extractor<32> be(bits);

	assertEquals( 0x11d0, be.extract(0, 8) );
	assertEquals( 0x7e1f, be.extract(16, 24) );
	assertEquals( 0x111f, be.extract(0, 24) );
	assertEquals( 0x1fd0, be.extract(24, 8) );
}

TEST_CASE( "bitsetExtractorTest/testLargerValue", "[unit]" )
{
	std::bitset<81> bits;
	for (int i = 0; i < 81; ++i)
		if (i % 3 == 0)
			bits.set(i);

	bitset_extractor<81> be(bits);
	uint64_t res = be.extract(0, 9, 18, 27, 36, 45, 54, 63);
	assertEquals( 0x2424242424242424, res );
}

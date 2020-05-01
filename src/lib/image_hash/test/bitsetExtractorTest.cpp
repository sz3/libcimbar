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
	bitset_extractor be(bits);

	assertEquals( 0x11d0, be.extract(0, 8) );
	assertEquals( 0x7e1f, be.extract(16, 24) );
	assertEquals( 0x111f, be.extract(0, 24) );
	assertEquals( 0x1fd0, be.extract(24, 8) );
}

TEST_CASE( "bitsetExtractorTest/testLargerValue.1", "[unit]" )
{
	std::bitset<81> bits;
	for (int i = 0; i < 81; ++i)
		if (i % 3 == 0)
			bits.set(i);

	bitset_extractor be(bits);
	uint64_t res = be.extract(0, 9, 18, 27, 36, 45, 54, 63);
	assertEquals( 0x2424242424242424, res );
}

TEST_CASE( "bitsetExtractorTest/testLargerValue.2", "[unit]" )
{
	std::bitset<100> bits("1111111110111111110011111110001111110000111110000011110000001110000000110000000010000000000000000000");
	bitset_extractor be(bits);
	uint64_t res = be.extract(1, 11, 21, 31);
	assertEquals( 0xfffefcf8, res );

	res = be.extract(41, 51, 61, 71);
	assertEquals( 0xf0e0c080, res );

	res = be.extract(1, 11, 21, 31, 41, 51, 61, 71);
	assertEquals( 0xfffefcf8f0e0c080ULL, res );

	res = be.extract(22, 32, 42, 52);
	assertEquals( 0xf8f0e0c0, res );

	res = be.extract(22, 32, 42, 52, 62, 72, 82, 92);
	assertEquals( 0xf8f0e0c080000000ULL, res );
}

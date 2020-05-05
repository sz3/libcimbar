#include "unittest.h"

#include "bit_extractor.h"
#include "intx/int128.hpp"

#include <bitset>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;

TEST_CASE( "bitExtractorTest/testDefault", "[unit]" )
{
	intx::uint128 bits = 0x11d07e1f;
	bit_extractor<intx::uint128, 32> be(bits);

	assertEquals( 0x11d0, be.extract(0, 8) );
	assertEquals( 0x7e1f, be.extract(16, 24) );
	assertEquals( 0x111f, be.extract(0, 24) );
	assertEquals( 0x1fd0, be.extract(24, 8) );
}

TEST_CASE( "bitExtractorTest/testLargerValue.1", "[unit]" )
{
	intx::uint128 bits(0);
	for (int i = 0; i < 81; ++i)
		bits = (bits << 1) | (i%3 == 0);

	bit_extractor<intx::uint128, 81> be(bits);
	uint64_t res = be.extract(0, 9, 18, 27, 36, 45, 54, 63);
	assertEquals( 0x9292929292929292, res );
}

TEST_CASE( "bitExtractorTest/testLargerValue.2", "[unit]" )
{
	intx::uint128 bits{0xFFBFCFE3FULL, 0xF83C0E030080000ULL};
	bit_extractor<intx::uint128, 100> be(bits);
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


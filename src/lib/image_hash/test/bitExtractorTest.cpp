/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "bit_extractor.h"
#include "intx/intx.hpp"
#include "serialize/format.h"

#include <string>
#include <tuple>

namespace {
	template<typename... T>
	std::string tuple_to_str(const std::tuple<T...>& tup)
	{
		std::string res;
		std::apply([&res](auto&&... args) {((res += fmt::format("{} ", args)), ...);}, tup);
		if (res.size() > 0)
			res = res.substr(0, res.size()-1);
		return res;
	}
}

TEST_CASE( "bitExtractorTest/testDefault.4", "[unit]" )
{
	uint64_t bits = 0x11d07e1f;
	bit_extractor<uint64_t, 32, 4> be(bits);

	// we're just reading 4 bits at a time.
	assertEquals( 0x1d, be.extract(0, 8) );    // 0001 1101
	assertEquals( 0x71, be.extract(16, 24) );  // 0x7 0x1
	assertEquals( 0x1e, be.extract(0, 20) );
	assertEquals( 0x0f, be.extract(12, 28) );
	assertEquals( 0x1d, be.extract(24, 8) );
}


TEST_CASE( "bitExtractorTest/testDefault.5", "[unit]" )
{
	uint64_t bits = 0x11d07e1f;
	bit_extractor<uint64_t, 32, 5> be(bits);

	assertEquals(  0x48, be.extract(0, 10) );   // 00010 01000
	assertEquals(  0xfc, be.extract(15, 20) );  // 00111 11100
	assertEquals(  0x9c, be.extract(1, 20) );   // 00100 11100
	assertEquals( 0x1e7, be.extract(16, 25) );  // 01111 00111
}

TEST_CASE( "bitExtractorTest/testDefault.8", "[unit]" )
{
	uint64_t bits = 0x11d07e1f;
	bit_extractor<uint64_t, 32, 8> be(bits);

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

	bit_extractor<intx::uint128, 81, 8> be(bits);
	uint64_t res = be.extract(0, 9, 18, 27, 36, 45, 54, 63);
	assertEquals( 0x9292929292929292, res );
}

TEST_CASE( "bitExtractorTest/testLargerValue.2", "[unit]" )
{
	intx::uint128 bits{0xF83C0E030080000ULL, 0xFFBFCFE3FULL};
	assertEquals( "ffbfcfe3f0f83c0e030080000", intx::hex(bits) );  // sanity check

	bit_extractor<intx::uint128, 100, 8> be(bits);
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

TEST_CASE( "bitExtractorTest/testTuple.5", "[unit]" )
{
	uint64_t bits = 0x11d07e1f;
	bit_extractor<uint64_t, 32, 5> be(bits);

	auto tup = be.pattern(0);
	assertEquals( "0 7 14 21 28", tuple_to_str(tup) );

	assertEquals(  0x2e8f00, be.extract(0, 7, 14, 21, 28) );
	assertEquals(  0x2e8f00, be.extract_tuple(tup) );
}

TEST_CASE( "bitExtractorTest/testTuple.8", "[unit]" )
{
	intx::uint128 bits{0xF83C0E030080000ULL, 0xFFBFCFE3FULL};
	bit_extractor<intx::uint128, 100, 8> be(bits);

	auto tup = be.pattern(3);
	assertEquals( "10 20 30 40 50 60 70 80", tuple_to_str(tup) );

	assertEquals(  0xfffefcf8f0e0c080, be.extract(10, 20, 30, 40, 50, 60, 70, 80) );
	assertEquals(  0xfffefcf8f0e0c080, be.extract_tuple(tup) );
}

TEST_CASE( "bitExtractorTest/testTuplePatterns.5", "[unit]" )
{
	uint64_t bits = 0x11d07e1f;
	bit_extractor<uint64_t, 32, 5> be(bits);

	std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned> tup = be.pattern(0);
	assertEquals( "0 7 14 21 28", tuple_to_str(tup) );
	assertEquals( "0 7 14 21 28", tuple_to_str(be.get_offsets(0)) );

	assertEquals( "1 8 15 22 29", tuple_to_str(be.pattern(1)) );
	assertEquals( "1 8 15 22 29", tuple_to_str(be.get_offsets(1)) );

	assertEquals( "2 9 16 23 30", tuple_to_str(be.pattern(2)) );
	assertEquals( "2 9 16 23 30", tuple_to_str(be.get_offsets(2)) );

	assertEquals( "7 14 21 28 35", tuple_to_str(be.pattern(3)) );
	assertEquals( "7 14 21 28 35", tuple_to_str(be.get_offsets(7)) );

	assertEquals( "8 15 22 29 36", tuple_to_str(be.pattern(4)) );
	assertEquals( "8 15 22 29 36", tuple_to_str(be.get_offsets(8)) );

	assertEquals( "9 16 23 30 37", tuple_to_str(be.pattern(5)) );
	assertEquals( "9 16 23 30 37", tuple_to_str(be.get_offsets(9)) );

	assertEquals( "14 21 28 35 42", tuple_to_str(be.pattern(6)) );
	assertEquals( "14 21 28 35 42", tuple_to_str(be.get_offsets(14)) );

	assertEquals( "15 22 29 36 43", tuple_to_str(be.pattern(7)) );
	assertEquals( "15 22 29 36 43", tuple_to_str(be.get_offsets(15)) );

	assertEquals( "16 23 30 37 44", tuple_to_str(be.pattern(8)) );
	assertEquals( "16 23 30 37 44", tuple_to_str(be.get_offsets(16)) );
}

TEST_CASE( "bitExtractorTest/testTuplePatterns.8", "[unit]" )
{
	intx::uint128 bits{0xF83C0E030080000ULL, 0xFFBFCFE3FULL};
	bit_extractor<intx::uint128, 100, 8> be(bits);

	std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned> tup = be.pattern(0);
	assertEquals( "0 10 20 30 40 50 60 70", tuple_to_str(tup) );
	assertEquals( "0 10 20 30 40 50 60 70", tuple_to_str(be.get_offsets(0)) );

	assertEquals( "1 11 21 31 41 51 61 71", tuple_to_str(be.pattern(1)) );
	assertEquals( "1 11 21 31 41 51 61 71", tuple_to_str(be.get_offsets(1)) );

	assertEquals( "2 12 22 32 42 52 62 72", tuple_to_str(be.pattern(2)) );
	assertEquals( "2 12 22 32 42 52 62 72", tuple_to_str(be.get_offsets(2)) );

	assertEquals( "10 20 30 40 50 60 70 80", tuple_to_str(be.pattern(3)) );
	assertEquals( "10 20 30 40 50 60 70 80", tuple_to_str(be.get_offsets(10)) );

	assertEquals( "11 21 31 41 51 61 71 81", tuple_to_str(be.pattern(4)) );
	assertEquals( "11 21 31 41 51 61 71 81", tuple_to_str(be.get_offsets(11)) );

	assertEquals( "12 22 32 42 52 62 72 82", tuple_to_str(be.pattern(5)) );
	assertEquals( "12 22 32 42 52 62 72 82", tuple_to_str(be.get_offsets(12)) );

	assertEquals( "20 30 40 50 60 70 80 90", tuple_to_str(be.pattern(6)) );
	assertEquals( "20 30 40 50 60 70 80 90", tuple_to_str(be.get_offsets(20)) );

	assertEquals( "21 31 41 51 61 71 81 91", tuple_to_str(be.pattern(7)) );
	assertEquals( "21 31 41 51 61 71 81 91", tuple_to_str(be.get_offsets(21)) );

	assertEquals( "22 32 42 52 62 72 82 92", tuple_to_str(be.pattern(8)) );
	assertEquals( "22 32 42 52 62 72 82 92", tuple_to_str(be.get_offsets(22)) );
}

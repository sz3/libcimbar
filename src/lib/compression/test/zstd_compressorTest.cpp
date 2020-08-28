/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "zstd_compressor.h"
#include "zstd_decompressor.h"

#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace cimbar;
using namespace std;

namespace {
	using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char>;

	string big_random(size_t size)
	{
		random_bytes_engine rbe;
		std::string data;
		data.resize(size);
		std::generate(begin(data), end(data), std::ref(rbe));
		return data;
	}
}

TEST_CASE( "zstd_compressorTest/testCompress", "[unit]" )
{
	std::stringstream ss;
	for (int i = 0; i < 1000; i+=10)
		ss << "0123456789";

	zstd_compressor<std::stringstream> comp;
	assertEquals( 1000, comp.compress(ss) );

	std::stringstream output;
	output << comp.rdbuf();

	assertEquals( 27, output.str().size() );

	char expected[] = "(\xb5" "/\xfd" "`\xe8\x02\x8d\x00\x00" "P0123456789\x01\x00\xdb[\x15$";
	assertEquals( 28, sizeof(expected) );
	assertEquals( std::string(expected, 27), output.str() );
}

TEST_CASE( "zstd_compressorTest/testRoundTrip.SmallRandom", "[unit]" )
{
	const int SIZE = 1000;
	std::stringstream ss;
	ss << big_random(SIZE);

	zstd_compressor<std::stringstream> comp;
	assertEquals( SIZE, comp.compress(ss) );

	std::stringstream output;
	output << comp.rdbuf();

	assertInRange( SIZE-100, output.str().size(), SIZE+100 );

	// point at decompressor
	zstd_decompressor<std::stringstream> dec;
	assertEquals( output.str().size(), dec.decompress(output) );

	std::stringstream recovered;
	recovered << dec.rdbuf();

	assertEquals( ss.str().size(), recovered.str().size() );
	assertEquals( ss.str(), recovered.str() );
}

TEST_CASE( "zstd_compressorTest/testRoundTrip.BigRandom", "[unit]" )
{
	const int SIZE = 100000;
	std::stringstream ss;
	ss << big_random(SIZE);

	zstd_compressor<std::stringstream> comp;
	assertEquals( SIZE, comp.compress(ss) );

	std::stringstream output;
	output << comp.rdbuf();

	assertInRange( SIZE-1000, output.str().size(), SIZE+1000 );

	// point at decompressor
	zstd_decompressor<std::stringstream> dec;
	assertEquals( output.str().size(), dec.decompress(output) );

	std::stringstream recovered;
	recovered << dec.rdbuf();

	assertEquals( ss.str().size(), recovered.str().size() );
	assertEquals( ss.str(), recovered.str() );
}

TEST_CASE( "zstd_compressorTest/testRoundTrip.BigRandom.Buffs", "[unit]" )
{
	const int SIZE = 100000;
	string original = big_random(SIZE);

	zstd_compressor<std::stringstream> comp;
	assertTrue( comp.write(original.data(), original.size()) );

	string output = comp.str();
	assertInRange( SIZE-1000, output.size(), SIZE+1000 );

	// point at decompressor
	zstd_decompressor<std::stringstream> dec;
	assertTrue( dec.write(output.data(), output.size()) );

	string recovered = dec.str();
	assertEquals( original.size(), recovered.size() );
	assertEquals( original, recovered );
}

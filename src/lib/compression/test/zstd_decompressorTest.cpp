/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "zstd_decompressor.h"

#include "zstd_compressor.h"
#include "serialize/format.h"
#include "util/File.h"
#include "util/MakeTempDirectory.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace cimbar;
using namespace std;
\
namespace {
	string big_repeated(size_t size, std::string tokens="aaaaa")
	{
		std::string data;
		while (data.size() < size)
			data += tokens;
		return data;
	}
}

TEST_CASE( "zstd_decompressorTest/testDecompress.Small", "[unit]" )
{
	char inputC[] = "(\xb5" "/\xfd" "`\xe8\x02\x8d\x00\x00" "P0123456789\x01\x00\xdb[\x15$";
	std::stringstream ss;
	ss << string(inputC, 27);

	std::stringstream expectedOutput;
	for (int i = 0; i < 1000; i+=10)
		expectedOutput << "0123456789";

	zstd_decompressor<std::stringstream> dec;
	assertEquals( 27, dec.decompress(ss) );

	std::stringstream output;
	output << dec.rdbuf();

	assertEquals( 1000, output.str().size() );
	assertEquals( expectedOutput.str(), output.str() );
}

TEST_CASE( "zstd_decompressorTest/testDecompress.Big", "[unit]" )
{
	char inputC[] = "(\xb5/\xfd\xa0\xa0\x86\x01\x00\x95\x00\x00" "P0123456789\x01\x00\x93\x86\xcd\x0b\x12";
	std::stringstream ss;
	ss << string(inputC, 30);

	std::stringstream expectedOutput;
	for (int i = 0; i < 100000; i+=10)
		expectedOutput << "0123456789";

	zstd_decompressor<std::stringstream> dec;
	assertEquals( 30, dec.decompress(ss) );

	std::stringstream output;
	output << dec.rdbuf();

	assertEquals( 100000, output.str().size() );
	assertEquals( expectedOutput.str(), output.str() );
}

TEST_CASE( "zstd_decompressorTest/testDecompress.FromBuff", "[unit]" )
{
	char inputC[] = "(\xb5/\xfd\xa0\xa0\x86\x01\x00\x95\x00\x00" "P0123456789\x01\x00\x93\x86\xcd\x0b\x12";
	string input(inputC, 30);

	std::stringstream expectedOutput;
	for (int i = 0; i < 100000; i+=10)
		expectedOutput << "0123456789";

	zstd_decompressor<std::stringstream> dec;
	assertTrue( dec.write(input.data(), input.size()) );

	std::stringstream output;
	output << dec.rdbuf();

	assertEquals( 100000, output.str().size() );
	assertEquals( expectedOutput.str(), output.str() );
}

TEST_CASE( "zstd_decompressorTest/testDecompress.ToFile", "[unit]" )
{
	MakeTempDirectory tempdir;

	char inputC[] = "(\xb5/\xfd\xa0\xa0\x86\x01\x00\x95\x00\x00" "P0123456789\x01\x00\x93\x86\xcd\x0b\x12";
	std::stringstream ss;
	ss << string(inputC, 30);

	std::stringstream expectedOutput;
	for (int i = 0; i < 100000; i+=10)
		expectedOutput << "0123456789";

	{
		zstd_decompressor<std::ofstream> dec(tempdir.path() / "decompress.txt");
		assertEquals( 30, dec.decompress(ss) );
	}

	string actual = File(tempdir.path() / "decompress.txt").read_all();
	assertEquals( expectedOutput.str(), actual );
}

TEST_CASE( "zstd_decompressorTest/testDecompress.Pad", "[unit]" )
{
	char inputC[] = "(\xb5" "/\xfd" "`\xe8\x02\x8d\x00\x00" "P0123456789\x01\x00\xdb[\x15$";
	std::stringstream ss;
	ss << string(inputC, 27);

	std::stringstream expectedOutput;
	for (int i = 0; i < 1000; i+=10)
		expectedOutput << "0123456789";

	zstd_decompressor<std::stringstream> dec;
	assertEquals( 27, dec.decompress(ss) );

	std::stringstream output;
	output << dec.rdbuf();

	assertEquals( 1000, output.str().size() );
	assertEquals( expectedOutput.str(), output.str() );
}

TEST_CASE( "zstd_decompressorTest/testDecompress.SmallBomb", "[unit]" )
{
	// it's sub 50 MB, so it's chill
	string rawBytes = big_repeated(48'000'000);
	zstd_compressor<std::stringstream> cpr;
	cpr.set_compression_level(22);
	assertTrue( cpr.write(rawBytes.data(), rawBytes.size()) );
	assertEquals( 52740, cpr.str().size() );

	zstd_decompressor<std::stringstream> dec;
	assertEquals( 52740, dec.decompress(cpr) );

	std::stringstream output;
	output << dec.rdbuf();

	assertEquals( rawBytes.size(), output.str().size() );
	//assertEquals( rawBytes.str(), output.str() );

	assertFalse( dec.good() );
	assertEquals( "suspicious compression ratio", dec.last_error() );
}

TEST_CASE( "zstd_decompressorTest/testDecompress.BigBomb", "[unit]" )
{
	// it's over 50 MB, we complain if the compression ratio is suspiciously high
	string rawBytes = big_repeated(53'000'000);
	zstd_compressor<std::stringstream> cpr;
	cpr.set_compression_level(22);
	assertTrue( cpr.write(rawBytes.data(), rawBytes.size()) );
	assertEquals( 58230, cpr.str().size() );

	zstd_decompressor<std::stringstream> dec;
	assertEquals( 58230, dec.decompress(cpr) );

	std::stringstream output;
	output << dec.rdbuf();

	assertEquals( 50003968, output.str().size() );
	//assertEquals( rawBytes.str(), output.str() );

	assertFalse( dec.good() );
	assertEquals( " detected suspicious compression ratio. Is someone up to no good?", dec.last_error() );
}


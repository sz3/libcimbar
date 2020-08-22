/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "zstd_decompressor.h"

#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

TEST_CASE( "zstd_decompressorTest/testDecompress.Small", "[unit]" )
{
	char inputC[] = "(\xb5" "/\xfd" "`\xe8\x02\x8d\x00\x00" "P0123456789\x01\x00\xdb[\x15$";
	std::stringstream ss;
	ss << string(inputC, 27);

	std::stringstream expectedOutput;
	for (int i = 0; i < 1000; i+=10)
		expectedOutput << "0123456789";

	zstd_decompressor dec;
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

	zstd_decompressor dec;
	assertEquals( 30, dec.decompress(ss) );

	std::stringstream output;
	output << dec.rdbuf();

	assertEquals( 100000, output.str().size() );
	assertEquals( expectedOutput.str(), output.str() );
}

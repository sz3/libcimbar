/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "encoder/reed_solomon_stream.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

namespace {
	string exampleDecodedBlock()
	{
		string ex = "01234567890123456789012345678901234567890123456789012345678901234567890123456789"
				"012345678901234567890123456789012345678901234567890123456789";
		return ex;
	}

	string exampleEncodedBlock155()
	{
		string ex = exampleDecodedBlock() + "\xa4t\x02\x03r\xc3\xad\xf2`\xc5\xb6\x9e&xs";
		return ex;
	}
}

TEST_CASE( "reed_solomon_streamTest/testEncodeOnce", "[unit]" )
{
	const unsigned size = 1000;
	string input;
	while (input.size() < size)
		input += "0123456789";

	stringstream ins(input);
	reed_solomon_stream<stringstream> rss(ins, 15, 155);

	assertEquals( 155, rss.readsome() );
	assertEquals( exampleEncodedBlock155(), string(rss.buffer(), 155) );
}

TEST_CASE( "reed_solomon_streamTest/testDecodeOnce", "[unit]" )
{
	stringstream outs;
	reed_solomon_stream<stringstream> rss(outs, 15, 155);

	string encoded = exampleEncodedBlock155();
	rss.write(encoded.data(), encoded.size());

	string actual = outs.str();
	assertEquals( 140, actual.size() );
	assertEquals( exampleDecodedBlock(), actual );
}

TEST_CASE( "reed_solomon_streamTest/testDecodeBad", "[unit]" )
{
	stringstream outs;
	reed_solomon_stream<stringstream> rss(outs, 15, 155);

	string encoded = string(155, 'f');
	rss.write(encoded.data(), encoded.size());

	string actual = outs.str();
	assertEquals( 140, actual.size() );
	assertEquals( string(140, '\0'), actual );
}


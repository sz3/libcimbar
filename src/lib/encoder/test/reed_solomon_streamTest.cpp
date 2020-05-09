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

	string exampleEncodedBlock()
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
	reed_solomon_stream<stringstream> rss(ins, 15);

	assertEquals( 155, rss.readsome() );
	assertEquals( exampleEncodedBlock(), string(rss.buffer(), 155) );
}

TEST_CASE( "reed_solomon_streamTest/testDecodeOnce", "[unit]" )
{
	stringstream outs;
	reed_solomon_stream<stringstream> rss(outs, 15);

	string encoded = exampleEncodedBlock();
	rss.write(encoded.data(), encoded.size());

	string actual = outs.str();
	assertEquals( 140, actual.size() );
	assertEquals( exampleDecodedBlock(), actual );
}


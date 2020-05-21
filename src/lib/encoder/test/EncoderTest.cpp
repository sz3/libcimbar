#include "unittest.h"

#include "encoder/Encoder.h"
#include "fountain/FountainInit.h"
#include "util/File.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
	//void makeTempFile(string
}

TEST_CASE( "EncoderTest/testVanilla", "[unit]" )
{
	std::string input = "Hello world";
	File f("/tmp/test.txt");
	f.write(input.data(), input.size());

	Encoder enc(4, 2);
	assertEquals( 2, enc.encode("/tmp/test.txt", "/tmp/doesntmatteryet.txt") );
}

TEST_CASE( "EncoderTest/testFountain", "[unit]" )
{
	FountainInit::init();

	std::string input = "Hello world";
	File f("/tmp/test.txt");
	f.write(input.data(), input.size());

	Encoder enc(4, 2);
	assertEquals( 3, enc.encode_fountain("/tmp/test.txt", "/tmp/forthefans.txt") );
}

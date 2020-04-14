#include "unittest.h"

#include "encoder/Encoder.h"

#include "cimb_translator/CimbWriter.h"
#include "util/File.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "EncoderIntegrationTest/testDefault", "[unit]" )
{
	std::string input = "Hello world";
	File f("/tmp/test.txt");
	f.write(input.data(), input.size());

	CimbWriter cw; // this currently has its own copy of encoder/color bits ... not ideal!
	Encoder enc(cw, 4, 2);
	enc.encode("/tmp/test.txt", "/tmp/howwedoin.png");
}

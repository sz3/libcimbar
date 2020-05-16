#include "unittest.h"

#include "encoder/Encoder.h"
#include "util/File.h"

#include "mock/MockCimbWriter.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "EncoderTest/testDefault", "[unit]" )
{
	std::string input = "Hello world";
	File f("/tmp/test.txt");
	f.write(input.data(), input.size());

	MockCimbWriter cw;
	Encoder enc(cw, 4, 2);
	assertEquals( 2, enc.encode("/tmp/test.txt", "/tmp/doesntmatteryet.txt") );
}

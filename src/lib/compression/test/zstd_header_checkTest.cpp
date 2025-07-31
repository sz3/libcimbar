/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "zstd_compressor.h"
#include "zstd_header_check.h"

#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace cimbar;

TEST_CASE( "zstd_header_checkTest/testGetFilename", "[unit]" )
{
	zstd_compressor<std::stringstream> comp;
	std::string expectedfn = "foobar.txt";
	assertEquals( 19, comp.write_header(expectedfn.data(), expectedfn.size()) );

	std::stringstream output;
	output << comp.rdbuf();

	assertEquals( 19, output.str().size() );

	char zstdblob[] = "\x50\x2A\x4D\x18\x0b\x00\x00\x00\x01" "foobar.txt";
	assertEquals( 20, sizeof(zstdblob) );
	assertEquals( std::string(zstdblob, sizeof(zstdblob)-1), output.str() );

	std::string actualfn = zstd_header_check::get_filename(reinterpret_cast<unsigned char*>(zstdblob), sizeof(zstdblob)-1);
	assertEquals( actualfn, expectedfn );
}

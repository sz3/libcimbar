/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "cimb_translator/Config.h"
#include "cimbar_js/cimbar_js.h"
#include "cimbar_js/cimbar_recv_js.h"
#include "cimbar_js/cimbar_zstd_js.h"
#include "serialize/format.h"

#include <iostream>
#include <string>

namespace {
	// misc
	std::string random_string(unsigned len, std::string chars="abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890")
	{
		std::cout << ::rand() << std::endl;
		std::string output = "";
		for (unsigned i = 0; i < len; ++i)
		{
			unsigned idx = ::rand() % chars.length();
			output += chars[idx];
		}
		return output;
	}
}

TEST_CASE( "cimbar_jsTest/testRoundtrip", "[unit]" )
{
	std::vector<unsigned char> decbuff;
	decbuff.resize(cimbard_get_bufsize());

	const int SIZE = 7000;
	std::string contents = random_string(SIZE);
	std::string filename = "/tmp/foobar-c语言版.txt";
	assertEquals( 0, cimbare_encode(reinterpret_cast<unsigned char*>(contents.data()), contents.size(), filename.data(), filename.size(), 100) );

	assertEquals( 1, cimbare_next_frame() );

	unsigned char* imgbuff;
	int imgsize = cimbare_get_frame_buff(&imgbuff);
	assertEquals( 1024*1024*3, imgsize );

	int bytes = cimbard_scan_extract_decode(imgbuff, 1024, 1024, 3, decbuff.data(), decbuff.size());
	assertEquals(bytes, 7500);

	unsigned chunkSize = cimbar::Config::fountain_chunk_size();
	assertEquals(0, bytes % chunkSize);

	int64_t res = cimbard_fountain_decode(decbuff.data(), bytes);
	assertTrue( res > 0 );

	// when res > 0
	{
		uint32_t fileId = res;

		std::string actualFilename;
		actualFilename.resize(255);
		int fnsz = cimbard_get_filename(fileId, actualFilename.data(), actualFilename.size());
		assertEquals( 21, fnsz );
		actualFilename.resize(fnsz);
		assertEquals( "foobar-c语言版.txt", actualFilename );

		std::vector<unsigned char> zstdbuff;
		zstdbuff.resize(cimbarz_get_bufsize());

		int outsize = cimbard_decompress_read(fileId, zstdbuff.data(), zstdbuff.size());
		assertEquals(contents.size(), outsize);
		std::string_view finalOutput{reinterpret_cast<char*>(zstdbuff.data()), contents.size()};

		assertEquals(contents, finalOutput);
	}

}


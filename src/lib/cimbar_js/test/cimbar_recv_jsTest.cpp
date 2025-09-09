/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "cimb_translator/Config.h"
//#include "cimbar_js/cimbar_js.h"
#include "cimbar_js/cimbar_recv_js.h"
#include "compression/zstd_decompressor.h"
#include "encoder/Encoder.h"
#include "serialize/format.h"
#include "util/byte_istream.h"

#include <iostream>
#include <string>

namespace {
	// a simple api for wirehair encoding
	// might eventually expose something like these in the cimbare_* namespace
	fountain_encoder_stream::ptr simp_wirehair_encode_init(const unsigned char* buff, unsigned size, const char* filename, unsigned fnsize, int compression_level)
	{
		Encoder enc; // defaults
		cimbar::byte_istream bis(reinterpret_cast<const char*>(buff), size);
		return enc.create_fountain_encoder(bis, std::string_view(filename, fnsize), compression_level);
	}

	int simp_wirehair_write(fountain_encoder_stream& fes, unsigned char* buff, unsigned size)
	{
		unsigned chunkSize = cimbar::Config::fountain_chunk_size();

		unsigned written = 0;
		while (size >= chunkSize)
		{
			fes.readsome(reinterpret_cast<char*>(buff+written), chunkSize);
			size -= chunkSize;
			written += chunkSize;
		}
		return written;
	}

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

TEST_CASE( "cimbar_recv_jsTest/testWirehairReassemble", "[unit]" )
{
	std::vector<unsigned char> buff;
	buff.resize(cimbard_get_bufsize());

	const int SIZE = 200000;
	std::string contents = random_string(SIZE);

	fountain_encoder_stream::ptr fes = simp_wirehair_encode_init(
				reinterpret_cast<unsigned char*>(contents.data()), contents.size(),
				nullptr, 0, 0);
	assertTrue( fes );

	int64_t dec = 0;
	for (int i = 0; i < 2000; ++i)
	{
		assertEquals(buff.size(), simp_wirehair_write(*fes, buff.data(), buff.size()));
		if (i % 2 == 1)
		{
			dec = cimbard_fountain_decode(buff.data(), buff.size());
			assertTrue(dec >= 0);
			if (dec > 0)
			{
				std::cout << "i: " << i << ", dec: " << dec << std::endl;
				break;
			}
		}
	}

	assertTrue(dec > 0);
	assertEquals(SIZE, cimbard_get_filesize(dec));

	// first call to cimbard_get_filename or cimbard_decompress_read() reassembles file
	std::string actualFilename;
	actualFilename.resize(255);
	assertEquals(0, cimbard_get_filename(dec, actualFilename.data(), actualFilename.size())); // no filename though

	unsigned char* data = cimbard_get_reassembled_file_buff();
	assertFalse( data == nullptr );
	std::string_view reassembled(reinterpret_cast<const char*>(data), SIZE);
	assertEquals(contents, reassembled);
}

TEST_CASE( "cimbar_recv_jsTest/testFullDecode", "[unit]" )
{
	std::vector<unsigned char> buff;
	buff.resize(cimbard_get_bufsize());

	cv::Mat img = TestCimbar::loadSample("b/4cecc30f.png");

	int bytes = cimbard_scan_extract_decode(img.data, img.cols, img.rows, 3, buff.data(), buff.size());
	assertEquals(bytes, 7500);

	unsigned chunkSize = cimbar::Config::fountain_chunk_size();
	assertEquals(0, bytes % chunkSize);

	int64_t res = cimbard_fountain_decode(buff.data(), bytes);
	assertTrue( res > 0 );

	// when res > 0
	{
		uint32_t fileId = res;

		assertEquals( 7347, cimbard_get_filesize(fileId) );

		unsigned size = cimbard_get_decompress_bufsize();
		assertEquals( 131072, size ); // defined by zstd. sanity check

		std::vector<unsigned char> data;
		data.resize(size);

		std::stringstream ss;
		int res = 1;
		while (res > 0)
		{
			res = cimbard_decompress_read(fileId, data.data(), data.size());
			if (res == 0)
				break;
			assertTrue( res > 0 );
			ss.write(reinterpret_cast<const char*>(data.data()), res);
		}

		assertEquals( 7538, ss.str().size() ); // decompressed
	}

}


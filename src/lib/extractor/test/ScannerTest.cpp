#include "unittest.h"

#include "Scanner.h"
#include "serialize/str_join.h"
#include <iostream>
#include <string>
#include <vector>

namespace {
	std::string get_sample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}
}

TEST_CASE( "ScannerTest/testPiecemealScan", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.t1_scan_rows();
	std::string res = turbo::str::join(candidates);
	assertStringContains("x=106-230,y=952-952", res);
	assertStringContains("x=110-233,y=1003-1003", res);
	assertStringContains("x=2340-2478,y=884-884", res);
	assertStringContains("x=2339-2477,y=918-918", res);
	assertStringContains("x=2282-2406,y=3043-3043", res);
	assertStringContains("x=2281-2404,y=3094-3094", res);
	assertStringContains("x=266-378,y=3009-3009", res);
	assertStringContains("x=270-381,y=3043-3043", res);

	candidates = sc.t2_scan_columns(candidates);
	assertStringContains("x=169-169,y=914-1047", turbo::str::join(candidates));
	assertStringContains("x=2409-2409,y=826-966", turbo::str::join(candidates));
	assertStringContains("x=2343-2343,y=3010-3123", turbo::str::join(candidates));
	assertStringContains("x=323-323,y=2977-3085", turbo::str::join(candidates));

	candidates = sc.t3_scan_diagonal(candidates);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "x=2342-2477,y=829-964 "
	    "x=104-236,y=916-1045 "
	    "x=2285-2403,y=3009-3125 "
	    "x=266-382,y=2975-3088",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "x=104-236,y=916-1045 "
	    "x=2342-2477,y=829-964 "
	    "x=266-382,y=2975-3088 "
	    "x=2285-2403,y=3009-3125",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSmallSample.1", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("myimage70.png"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "x=42-102,y=43-102 "
	    "x=911-956,y=55-100 "
	    "x=126-168,y=901-943 "
	    "x=891-929,y=839-877",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSmallSample.2", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("myimage110.png"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	// x=58-114,y=100-156 x=914-959,y=106-151 x=673-686,y=581-594 x=904-942,y=888-926
	assertEquals(
	    "x=58-114,y=100-156 "
	    "x=914-959,y=106-151 "
	    "x=129-171,y=949-991 "
	    "x=904-942,y=888-926",
	    turbo::str::join(candidates)
	);
}

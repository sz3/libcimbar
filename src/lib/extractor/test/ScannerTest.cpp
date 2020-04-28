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
	assertStringContains("x=106-233,y=952-1003", turbo::str::join(candidates));
	assertStringContains("x=2339-2479,y=867-918", turbo::str::join(candidates));
	assertStringContains("x=2281-2406,y=3043-3094", turbo::str::join(candidates));
	assertStringContains("x=266-381,y=3009-3043", turbo::str::join(candidates));

	candidates = sc.t2_scan_columns(candidates);
	assertStringContains("x=169-169,y=914-1046", turbo::str::join(candidates));
	assertStringContains("x=2409-2409,y=826-966", turbo::str::join(candidates));
	assertStringContains("x=2343-2343,y=3010-3123", turbo::str::join(candidates));
	assertStringContains("x=323-323,y=2977-3085", turbo::str::join(candidates));

	candidates = sc.t3_scan_diagonal(candidates);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "x=2342-2477,y=829-964 "
	    "x=105-234,y=916-1045 "
	    "x=2286-2402,y=3009-3125 "
	    "x=267-379,y=2975-3087",
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
	    "x=105-234,y=916-1045 "
	    "x=2342-2477,y=829-964 "
	    "x=267-379,y=2975-3087 "
	    "x=2286-2402,y=3009-3125",
	    turbo::str::join(candidates)
	);
}

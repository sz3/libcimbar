#include "unittest.h"

#include "Scanner.h"

#include "Corners.h"
#include "Point.h"
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
	cv::Mat img = cv::imread(get_sample("4color-ecc40-fountain.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.t1_scan_rows();
	std::string res = turbo::str::join(candidates);
	assertStringContains("68+-22,187+-0", res);
	assertStringContains("840+-23,153+-0", res);
	assertStringContains("147+-18,884+-0", res);
	assertStringContains("809+-19,884+-0", res);

	candidates = sc.t2_scan_columns(candidates);
	assertStringContains("68+-22,191+-23", turbo::str::join(candidates));
	assertStringContains("840+-23,156+-25", turbo::str::join(candidates));
	assertStringContains("147+-18,890+-17", turbo::str::join(candidates));
	assertStringContains("809+-19,883+-17", turbo::str::join(candidates));

	candidates = sc.t3_scan_diagonal(candidates);
	assertStringContains("69+-23,192+-23", turbo::str::join(candidates));
	assertStringContains("840+-22,156+-22", turbo::str::join(candidates));
	assertStringContains("147+-17,890+-17", turbo::str::join(candidates));
	assertStringContains("810+-18,884+-18", turbo::str::join(candidates));

	candidates = sc.t4_confirm_scan(candidates);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "840+-23,156+-25 69+-23,192+-24 148+-18,890+-17 809+-19,884+-18",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color-ecc40-fountain.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "69+-23,192+-24 "
	    "840+-23,156+-25 "
	    "148+-18,890+-17 "
	    "809+-19,884+-18",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testScanEdges", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color-ecc40-fountain.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "69+-23,192+-24 "
	    "840+-23,156+-25 "
	    "148+-18,890+-17 "
	    "809+-19,884+-18",
	    turbo::str::join(candidates)
	);

	Corners cs(candidates);
	std::vector<point<int>> edges = sc.scan_edges(cs);
	assertEquals( "446,151 846,548 472,906 90,567", turbo::str::join(edges) );
}


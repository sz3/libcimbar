#include "unittest.h"

#include "Scanner.h"

#include "Corners.h"
#include "Midpoints.h"
#include "Point.h"
#include "serialize/str_join.h"
#include <iostream>
#include <string>
#include <vector>




TEST_CASE( "ScannerTest/testPiecemealScan", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates;
	sc.t1_scan_rows([&candidates] (const Anchor& p) { candidates.push_back(p); });
	std::string res = turbo::str::join(candidates);
	assertStringContains("51+-26,288+-0", res);
	assertStringContains("992+-27,288+-0", res);
	assertStringContains("85+-24,1188+-0", res);
	assertStringContains("959+-25,1188+-0", res);

	std::vector<Anchor> c2;
	for (const Anchor& c : candidates)
		sc.t2_scan_column(c, [&c2] (const Anchor& p) { c2.push_back(p); });
	assertStringContains("51+-0,286+-28", turbo::str::join(c2));
	assertStringContains("992+-0,283+-28", turbo::str::join(c2));
	assertStringContains("85+-0,1188+-24", turbo::str::join(c2));
	assertStringContains("959+-0,1196+-24", turbo::str::join(c2));

	std::vector<Anchor> c3;
	for (const Anchor& c : c2)
		sc.t3_scan_diagonal(c, [&c3] (const Anchor& p) { c3.push_back(p); });
	assertStringContains("51+-27,286+-28", turbo::str::join(c3));
	assertStringContains("992+-26,283+-28", turbo::str::join(c3));
	assertStringContains("85+-24,1188+-24", turbo::str::join(c3));
	assertStringContains("958+-23,1196+-24", turbo::str::join(c3));

	std::vector<Anchor> c4;
	for (const Anchor& c : c3)
		sc.t4_confirm_scan(c, [&c4] (const Anchor& p) { c4.push_back(p); });

	candidates = sc.deduplicate_candidates(c4);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "992+-27,283+-28 51+-27,286+-28 85+-24,1188+-24 959+-25,1196+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "51+-27,286+-28 992+-27,283+-28 85+-24,1188+-24 959+-25,1196+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.2", "[unit]" )
{
	cv::Mat img = cv::imread("/opt/maindisk/Sonic/proj/prettyqr/samples/perf1c/scan76.png");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "235+-29,74+-28 1164+-24,102+-25 256+-28,1021+-26 1157+-24,980+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.3", "[unit]" )
{
	cv::Mat img = cv::imread("/opt/maindisk/Sonic/proj/prettyqr/samples/perf1c/scan137.png");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "241+-28,98+-28 "
	    "1163+-25,111+-26 "
	    "269+-27,1037+-27 "
	    "1168+-24,991+-25",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.4", "[unit]" )
{
	cv::Mat img = cv::imread("/opt/maindisk/Sonic/proj/prettyqr/samples/perf1c/scan103.png");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "215+-29,64+-29 1155+-25,89+-25 242+-28,1021+-26 1151+-24,973+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.5", "[unit]" )
{
	cv::Mat img = cv::imread("/opt/maindisk/Sonic/proj/prettyqr/samples/perf1c/rl-scan100.png");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "59+-27,245+-28 1014+-28,217+-29 107+-24,1152+-24 989+-25,1155+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testScanEdges", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "51+-27,286+-28 992+-27,283+-28 85+-24,1188+-24 959+-25,1196+-24",
	    turbo::str::join(candidates)
	);

	Corners cs(candidates);
	Midpoints mp;
	std::vector<point<int>> edges = sc.scan_edges(cs, mp);
	assertEquals( "518,255 1004,757 519,1219 41,754", turbo::str::join(edges) );

	// check "expected" midpoints as well.
	assertEquals( "518.489,284.51 974.887,756.458 519.403,1191.98 68.6238,753.548", turbo::str::join(mp.points()) );
}


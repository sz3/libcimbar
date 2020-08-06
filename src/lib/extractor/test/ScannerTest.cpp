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
	sc.t1_scan_rows<ScanState_114>([&candidates] (const Anchor& p) { candidates.push_back(p); });
	std::string res = turbo::str::join(candidates);
	assertStringContains("51+-26,288+-0", res);
	assertStringContains("992+-27,288+-0", res);
	assertStringContains("85+-24,1188+-0", res);
	assertStringContains("959+-25,1188+-0", res);

	std::vector<Anchor> c2;
	for (const Anchor& c : candidates)
		sc.t2_scan_column<ScanState_114>(c, [&c2] (const Anchor& p) { c2.push_back(p); });
	assertStringContains("51+-0,286+-28", turbo::str::join(c2));
	assertStringContains("992+-0,283+-28", turbo::str::join(c2));
	assertStringContains("85+-0,1188+-24", turbo::str::join(c2));
	assertStringContains("959+-0,1196+-24", turbo::str::join(c2));

	std::vector<Anchor> c3;
	for (const Anchor& c : c2)
		sc.t3_scan_diagonal<ScanState_114>(c, [&c3] (const Anchor& p) { c3.push_back(p); });
	assertStringContains("51+-27,286+-28", turbo::str::join(c3));
	assertStringContains("992+-26,283+-28", turbo::str::join(c3));
	assertStringContains("85+-24,1188+-24", turbo::str::join(c3));
	assertStringContains("958+-23,1196+-24", turbo::str::join(c3));

	std::vector<Anchor> c4;
	for (const Anchor& c : c3)
		sc.t4_confirm_scan<ScanState_114>(c, [&c4] (const Anchor& p) { c4.push_back(p); });

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
	    "51+-28,285+-28 993+-28,282+-28 84+-25,1188+-24 959+-25,1196+-24",
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
	    "235+-29,74+-29 1164+-24,102+-26 255+-28,1021+-26 1157+-24,980+-24",
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
	    "240+-29,98+-29 1163+-25,111+-26 269+-28,1038+-27 1168+-24,991+-25",
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
	    "216+-30,64+-30 1155+-25,90+-26 242+-28,1021+-27 1151+-24,973+-24",
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
	    "59+-27,245+-28 1015+-29,216+-29 106+-24,1152+-24 990+-26,1156+-25",
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
	    "51+-28,285+-28 993+-28,282+-28 84+-25,1188+-24 959+-25,1196+-24",
	    turbo::str::join(candidates)
	);

	Corners cs(candidates);
	Midpoints mp;
	std::vector<point<int>> edges = sc.scan_edges(cs, mp);
	assertEquals( "518,255 1004,757 518,1219 40,754", turbo::str::join(edges) );

	// check "expected" midpoints as well.
	assertEquals( "518.99,283.51 975.369,755.954 518.903,1191.98 68.1046,753.045", turbo::str::join(mp.points()) );
}


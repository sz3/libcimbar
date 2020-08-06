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

	std::vector<Anchor> candidates = sc.t1_scan_rows<ScanState_114>();
	std::string res = turbo::str::join(candidates);
	assertStringContains("51+-27,288+-0", res);
	assertStringContains("993+-28,288+-0", res);
	assertStringContains("84+-25,1188+-0", res);
	assertStringContains("959+-26,1206+-0", res);

	candidates = sc.t2_scan_columns<ScanState_114>(candidates);
	assertStringContains("51+-0,285+-28", turbo::str::join(candidates));
	assertStringContains("993+-0,282+-28", turbo::str::join(candidates));
	assertStringContains("84+-0,1188+-24", turbo::str::join(candidates));
	assertStringContains("959+-0,1196+-24", turbo::str::join(candidates));

	candidates = sc.t3_scan_diagonal<ScanState_114>(candidates);
	assertStringContains("51+-28,285+-28", turbo::str::join(candidates));
	assertStringContains("993+-27,282+-28", turbo::str::join(candidates));
	assertStringContains("84+-24,1188+-24", turbo::str::join(candidates));
	assertStringContains("960+-24,1196+-24", turbo::str::join(candidates));

	candidates = sc.t4_confirm_scan<ScanState_114>(candidates);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "993+-28,282+-28 51+-28,285+-28 84+-25,1188+-24 959+-25,1196+-24",
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


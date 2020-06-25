
#include "encoder/Encoder.h"
#include "fountain/FountainInit.h"

#include "cxxopts/cxxopts.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar video encoder", "Draw a bunch of weird static on the screen!");

	options.add_options()
	    ("i,in", "Source file", cxxopts::value<vector<string>>())
	    ("e,ecc", "ECC level (default: 40)", cxxopts::value<unsigned>())
	    ("f,fps", "Target FPS (default: 30)", cxxopts::value<unsigned>())
	    ("h,help", "Print usage")
	;

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("in"))
	{
	  std::cout << options.help() << std::endl;
	  exit(0);
	}

	vector<string> infiles = result["in"].as<vector<string>>();

	unsigned ecc = 40;
	if (result.count("ecc"))
		ecc = result["ecc"].as<unsigned>();

	unsigned fps = 0;
	if (result.count("fps"))
		fps = result["fps"].as<unsigned>();
	if (fps == 0)
		fps = 30;
	unsigned delay = 1000 / fps;

	FountainInit::init();

	// we currently run until the user kills the parent program. Would be nice to base it off the opencv child window somehow?
	bool dark = true;
	cv::Scalar bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
	cv::Mat windowImg = cv::Mat(1080, 1080, CV_8UC3, bgcolor);
	auto draw = [windowImg, delay] (const cv::Mat& frame, unsigned) {
		frame.copyTo(windowImg(cv::Rect(28, 28, frame.cols, frame.rows)));
		cv::imshow("image", windowImg);
		cv::waitKey(delay); // functions as the frame delay... you can hold down a key to make it go faster
	};

	Encoder en(ecc);
	while (true)
		for (const string& f : infiles)
			en.encode_fountain(f, draw);
	return 0;
}


#include "encoder/Encoder.h"
#include "fountain/FountainInit.h"
#include "util/loop_iterator.h"

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
	    ("e,ecc", "ECC level (default: 64)", cxxopts::value<unsigned>())
	    ("f,fps", "Target FPS (default: 30)", cxxopts::value<unsigned>())
	    ("s,shakycam", "Successive images are offset, like a shaky camera effect", cxxopts::value<bool>())
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

	bool dark = true;
	cv::Scalar bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
	cv::Mat windowImg = cv::Mat(1080, 1080, CV_8UC3, bgcolor);

	bool running = true;
	bool start = true;

	bool shakycam = result.count("shakycam");
	using ShakeArr = std::array<std::pair<int, int>, 8>;
	ShakeArr shakePos = {{
	    {0, 0}, {-8, -8}, {0, 0}, {8, 8}, {0, 0}, {-8, 8}, {0, 0}, {8, -8}
	}};

	loop_iterator<ShakeArr, ShakeArr::const_iterator> shakeIt(shakePos);

	auto draw = [&windowImg, delay, bgcolor, shakycam, &running, &start, &shakeIt] (const cv::Mat& frame, unsigned) {
		if (!start and cv::getWindowProperty("image", cv::WND_PROP_AUTOSIZE) < 0)
			return running = false;

		start = false;

		int offsetX = 28;
		int offsetY = 28;
		if (shakycam)
		{
			windowImg = bgcolor;
			if (++shakeIt)
			{
				offsetX += (*shakeIt).first;
				offsetY += (*shakeIt).second;
			}
		}
		frame.copyTo(windowImg(cv::Rect(offsetX, offsetY, frame.cols, frame.rows)));
		cv::imshow("image", windowImg);
		cv::waitKey(delay); // functions as the frame delay... you can hold down a key to make it go faster
		return true;
	};

	Encoder en(ecc);
	while (running)
		for (const string& f : infiles)
			en.encode_fountain(f, draw);
	return 0;
}

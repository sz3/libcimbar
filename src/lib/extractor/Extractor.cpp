#include "Extractor.h"

#include "Deskewer.h"
#include "Scanner.h"
#include <vector>
using std::string;

Extractor::Extractor()
{
}

bool Extractor::extract(const cv::Mat& img, cv::Mat& out)
{
	Scanner scanner(img);
	std::vector<Anchor> points = scanner.scan();
	if (points.size() < 4)
		return false;

	Corners corners(points[0].center(), points[1].center(), points[2].center(), points[3].center());
	Deskewer de;
	out = de.deskew(img, corners);
	return true;
}

bool Extractor::extract(string read_path, cv::Mat& out)
{
	cv::Mat img = cv::imread(read_path);
	return extract(img, out);
}

bool Extractor::extract(string read_path, string write_path)
{
	cv::Mat img = cv::imread(read_path);
	cv::Mat out;
	bool res = extract(img, out);
	cv::imwrite(write_path, out);
	return res;
}

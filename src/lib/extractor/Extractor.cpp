#include "Extractor.h"

#include "Deskewer.h"
#include "Scanner.h"
#include <vector>
using std::string;

Extractor::Extractor()
{
}

bool Extractor::sharpen(cv::Mat& out)
{
	static const cv::Mat kernel = (cv::Mat_<char>(3,3) <<  -1, -1, -1, -1, 9, -1, -1, -1, -1);
	cv::filter2D(out, out, -1, kernel);
	return true;
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

	if (img.cols < out.cols or img.rows < out.rows)
		sharpen(out);
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

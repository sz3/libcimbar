#include "Extractor.h"

#include "Deskewer.h"
#include "Scanner.h"
#include <vector>
using std::string;

Extractor::Extractor()
{
}

int Extractor::extract(const cv::Mat& img, cv::Mat& out)
{
	Scanner scanner(img);
	std::vector<Anchor> points = scanner.scan();
	if (points.size() < 4)
		return FAILURE;

	Corners corners(points);
	Deskewer de;
	out = de.deskew(img, corners);

	if ( !corners.is_granular_scale(de.total_size()) )
		return NEEDS_SHARPEN;
	return SUCCESS;
}

int Extractor::extract(const cv::UMat& img, cv::UMat& out)
{
	Scanner scanner(img);
	std::vector<Anchor> points = scanner.scan();
	if (points.size() < 4)
		return FAILURE;

	Corners corners(points);
	Deskewer de;
	out = de.deskew(img, corners);

	if ( !corners.is_granular_scale(de.total_size()) )
		return NEEDS_SHARPEN;
	return SUCCESS;
}

int Extractor::extract(string read_path, cv::Mat& out)
{
	cv::Mat img = cv::imread(read_path);
	return extract(img, out);
}

int Extractor::extract(string read_path, string write_path)
{
	cv::UMat img = cv::imread(read_path).getUMat(cv::ACCESS_FAST); // cv::USAGE_ALLOCATE_SHARED_MEMORY would be nice...;
	int res = extract(img, img);
	cv::imwrite(write_path, img);
	return res;
}

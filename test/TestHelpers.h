
#include <opencv2/opencv.hpp>
#include "serialize/format.h"
#include <string>

namespace TestCimbar
{
	inline std::string getSample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}

	inline cv::Mat loadSample(std::string filename)
	{
		cv::Mat mat = cv::imread(getSample(filename));
		cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
		return mat;
	}

	inline std::string getProjectDir()
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT);
	}
}


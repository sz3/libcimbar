#pragma once

#include <opencv2/opencv.hpp>

#include <iostream>

// http://brucelindbloom.com/Eqn_ChromAdapt.html
// https://github.com/gtaylor/python-colormath

namespace von_kries
{

cv::Mat MA()
{
	static cv::Mat transform = (cv::Mat1d(3, 3) <<
	                            0.40024, 0.70760, -0.08081,
	                            -0.22630, 1.16532, 0.04570,
	                            0.00000, 0.00000, 0.91822
	);
	return transform;
}


cv::Mat get_adaptation_matrix(const std::tuple<double, double, double>& actual, const std::tuple<double, double, double>& desired)
{
	std::array<double, 3> a = { std::get<0>(actual), std::get<1>(actual), std::get<2>(actual)};
	cv::Mat src = cv::Mat(3, 1, CV_64F, a.data());
	std::array<double, 3> b = { std::get<0>(desired), std::get<1>(desired), std::get<2>(desired)};
	cv::Mat dst = cv::Mat(3, 1, CV_64F, b.data());

	cv::Mat m1 = MA() * src;
	cv::Mat m2 = MA() * dst;

	cv::Mat d = cv::Mat::diag(m2 / m1);
	return MA().inv() * d * MA();
}

std::tuple<double, double, double> transform(const cv::Mat& m, double r, double g, double b)
{
	std::array<double, 3> a = { r, g, b };
	cv::Mat temp = m * cv::Mat(3, 1, CV_64F, a.data());
	return {temp.at<double>(0), temp.at<double>(1), temp.at<double>(2)};
}


}

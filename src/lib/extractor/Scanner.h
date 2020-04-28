#pragma once

#include "Anchor.h"
#include "Corners.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class Scanner
{
public: // public interface
	Scanner(const cv::Mat& img, bool dark=true, int skip=17);

	std::vector<Anchor> scan();

public: // other interesting methods
	static cv::Mat preprocess_image(const cv::Mat& img);
	std::vector<Anchor> deduplicate_candidates(const std::vector<Anchor>& candidates) const;
	void filter_candidates(std::vector<Anchor>& candidates) const;

	std::vector<Anchor> t1_scan_rows() const;
	std::vector<Anchor> t2_scan_columns(const std::vector<Anchor>& candidates) const;
	std::vector<Anchor> t3_scan_diagonal(const std::vector<Anchor>& candidates) const;
	bool sort_top_to_bottom(std::vector<Anchor>& points);

protected: // internal member functions
	bool test_pixel(int x, int y) const;

	void scan_horizontal(std::vector<Anchor>& points, int y) const;
	void scan_vertical(std::vector<Anchor>& points, int x, int ystart=-1, int yend=-1) const;
	void scan_diagonal(std::vector<Anchor>& points, int xstart, int xend, int ystart, int yend) const;

protected:
	cv::Mat _img;
	bool _dark;
	int _skip;
};


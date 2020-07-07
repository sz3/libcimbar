#pragma once

#include "Anchor.h"
#include "Point.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class Corners;
class Midpoints;

class Scanner
{
public: // public interface
	Scanner(const cv::Mat& img, bool dark=true, int skip=0);
	Scanner(const cv::UMat& img, bool dark=true, int skip=0);
	int anchor_size() const;

	std::vector<Anchor> scan();
	std::vector<point<int>> scan_edges(const Corners& corners, Midpoints& mps) const;


public: // other interesting methods
	template <typename MAT>
	static MAT preprocess_image(const MAT& img);

	std::vector<Anchor> deduplicate_candidates(const std::vector<Anchor>& candidates) const;
	void filter_candidates(std::vector<Anchor>& candidates) const;

	std::vector<Anchor> t1_scan_rows() const;
	std::vector<Anchor> t2_scan_columns(const std::vector<Anchor>& candidates) const;
	std::vector<Anchor> t3_scan_diagonal(const std::vector<Anchor>& candidates) const;
	std::vector<Anchor> t4_confirm_scan(const std::vector<Anchor>& candidates) const;
	bool sort_top_to_bottom(std::vector<Anchor>& points);

public: // helpers
	static unsigned nextPowerOfTwoPlusOne(unsigned v);

protected: // internal member functions
	bool test_pixel(int x, int y) const;

	bool scan_horizontal(std::vector<Anchor>& points, int y, int xstart=-1, int xend=-1) const;
	bool scan_vertical(std::vector<Anchor>& points, int x, int xmax=-1, int ystart=-1, int yend=-1) const;
	bool scan_diagonal(std::vector<Anchor>& points, int xstart, int xend, int ystart, int yend) const;

	// edge detection
	bool chase_edge(const point<double>& start, const point<double>& unit) const;
	point<int> find_edge(const point<int>& u, const point<int>& v, point<double> mid) const;

protected:
	cv::Mat _img;
	bool _dark;
	int _skip;
	int _mergeCutoff;
	int _anchorSize;
};

inline unsigned Scanner::nextPowerOfTwoPlusOne(unsigned v)
{
	// get next power of 2 + 1
	// min 3
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return std::max(3U, v + 2);
}

template <typename MAT>
inline MAT Scanner::preprocess_image(const MAT& img)
{
	unsigned unitX = nextPowerOfTwoPlusOne((unsigned)(img.cols * 0.002));
	unsigned unitY = nextPowerOfTwoPlusOne((unsigned)(img.rows * 0.002));

	MAT out;
	if (img.channels() >= 3)
		cv::cvtColor(img, out, cv::COLOR_BGR2GRAY);
	else
		out = img;

	cv::GaussianBlur(out, out, cv::Size(unitY, unitX), 0);

	cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(4.0, cv::Size(100, 100));
	clahe->apply(out, out);

	cv::threshold(out, out, 127, 255, cv::THRESH_BINARY); // do we actually need this?
	return out;
}

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
public: // public inline methods
	template <typename MAT>
	Scanner(const MAT& img, bool dark=true, int skip=0);

	template <typename MAT>
	static cv::Mat threshold_clahe(const MAT& img);

	template <typename MAT>
	static cv::Mat threshold_fast(const MAT& img);

	template <typename MAT>
	static cv::Mat preprocess_image(const MAT& img, bool quick=true);

	static unsigned nextPowerOfTwoPlusOne(unsigned v); // helper

	// rest of public interface
	std::vector<Anchor> scan();
	std::vector<point<int>> scan_edges(const Corners& corners, Midpoints& mps) const;
	int anchor_size() const;

public: // other interesting methods
	std::vector<Anchor> deduplicate_candidates(const std::vector<Anchor>& candidates) const;
	void filter_candidates(std::vector<Anchor>& candidates) const;

	std::vector<Anchor> t1_scan_rows() const;
	std::vector<Anchor> t2_scan_columns(const std::vector<Anchor>& candidates) const;
	std::vector<Anchor> t3_scan_diagonal(const std::vector<Anchor>& candidates) const;
	std::vector<Anchor> t4_confirm_scan(const std::vector<Anchor>& candidates) const;
	bool sort_top_to_bottom(std::vector<Anchor>& points);

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
inline cv::Mat Scanner::threshold_clahe(const MAT& img)
{
	cv::Mat res;
	cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(4.0, cv::Size(100, 100));
	clahe->apply(img, res);

	cv::threshold(res, res, 127, 255, cv::THRESH_BINARY);
	return res;
}

template <typename MAT>
inline cv::Mat Scanner::threshold_fast(const MAT& img)
{
	unsigned unit = std::min(img.cols, img.rows);
	unit = nextPowerOfTwoPlusOne((unsigned)(unit * 0.05));
	cv::Mat res;
	cv::adaptiveThreshold(img, res, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, unit, 0);
	return res;
}

template <typename MAT>
inline cv::Mat Scanner::preprocess_image(const MAT& img, bool quick)
{
	unsigned unitX = nextPowerOfTwoPlusOne((unsigned)(img.cols * 0.002));
	unsigned unitY = nextPowerOfTwoPlusOne((unsigned)(img.rows * 0.002));

	MAT out;
	if (img.channels() >= 3)
		cv::cvtColor(img, out, cv::COLOR_BGR2GRAY);
	else
		out = img.clone();

	cv::GaussianBlur(out, out, cv::Size(unitY, unitX), 0);
	if (quick)
		return threshold_fast(out);
	else
		return threshold_clahe(out);
}

template <typename MAT>
inline Scanner::Scanner(const MAT& img, bool dark, int skip)
    : _dark(dark)
    , _skip(skip? skip : std::min(img.rows, img.cols) / 60)
    , _mergeCutoff(img.cols / 30)
    , _anchorSize(30)
{
	_img = preprocess_image(img);
}

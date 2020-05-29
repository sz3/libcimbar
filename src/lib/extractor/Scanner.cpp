#include "Scanner.h"

#include "ScanState.h"
#include "serialize/format.h"
#include <algorithm>

namespace {
	struct size_sort
	{
		inline bool operator() (const Anchor& an1, const Anchor& an2)
		{
			return an1.size() > an2.size();
		}
	};

	unsigned nextPowerOfTwoPlusOne(unsigned v)
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
}

Scanner::Scanner(const cv::Mat& img, bool dark, int skip)
    : _dark(dark)
    , _skip(skip)
    , _mergeCutoff(img.cols / 34)
{
	_img = preprocess_image(img);
}

cv::Mat Scanner::preprocess_image(const cv::Mat& img)
{
	unsigned unitX = nextPowerOfTwoPlusOne((unsigned)(img.cols * 0.002));
	unsigned unitY = nextPowerOfTwoPlusOne((unsigned)(img.rows * 0.002));

	cv::Mat out;
	if (img.channels() >= 3)
		cv::cvtColor(img, out, cv::COLOR_BGR2GRAY);
	else
		out = img;

	cv::GaussianBlur(out, out, cv::Size(unitY, unitX), 0);

	cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(4.0, cv::Size(100, 100));
	clahe->apply(out, out);

	cv::threshold(out, out, 127, 255, cv::THRESH_BINARY);
	return out;
}

bool Scanner::test_pixel(int x, int y) const
{
	uchar pixel = _img.at<uchar>(y, x);
	if (_dark)
		return pixel > 127;
	else
		return pixel < 127;
}

std::vector<Anchor> Scanner::deduplicate_candidates(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> merged;
	for (const Anchor& c : candidates)
	{
		bool foundMerge = false;
		for (Anchor& m : merged)
		{
			if (m.within_merge_distance(c, _mergeCutoff))
			{
				foundMerge = true;
				m.merge(c);
				break;
			}
		}
		if (!foundMerge)
			merged.push_back(c);
	}
	return merged;
}

void Scanner::filter_candidates(std::vector<Anchor>& candidates) const
{
	// returns the best 4 candidates
	if (candidates.size() <= 4)
		return;

	std::sort(candidates.begin(), candidates.end(), size_sort());
	unsigned cutoff = 0;
	for (int i = 0; i < 4; ++i)
		cutoff += candidates[i].size();
	cutoff /= 8; // avg / 2

	int i = 0;
	for (; i < candidates.size(); ++i)
		if (candidates[i].size() < cutoff)
			break;
	if (i < candidates.size())
		candidates.resize(i);
}

bool Scanner::scan_horizontal(std::vector<Anchor>& points, int y, int xstart, int xend) const
{
	if (xstart < 0)
		xstart = 0;
	if (xend < 0 or xend > _img.cols)
		xend = _img.cols;

	unsigned initCount = points.size();
	ScanState state;
	for (int x = xstart; x < xend; ++x)
	{
		bool active = test_pixel(x, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(x-res, x-1, y, y));
	}

	// if the pattern is at the edge of the range
	int res = state.process(false);
	if (res > 0)
	{
		int x = xend;
		points.push_back(Anchor(x-res, x-1, y, y));
	}
	return initCount != points.size();
}

bool Scanner::scan_vertical(std::vector<Anchor>& points, int x, int ystart, int yend) const
{
	if (ystart < 0)
		ystart = 0;
	if (yend < 0 or yend > _img.rows)
		yend = _img.rows;

	unsigned initCount = points.size();
	ScanState state;
	for (int y = ystart; y < yend; ++y)
	{
		bool active = test_pixel(x, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(x, x, y-res, y-1));
	}

	// if the pattern is at the edge of the range
	int res = state.process(false);
	if (res > 0)
	{
		int y = yend;
		points.push_back(Anchor(x, x, y-res, y-1));
	}
	return initCount != points.size();
}

void Scanner::scan_diagonal(std::vector<Anchor>& points, int xstart, int xend, int ystart, int yend) const
{
	xend = std::min(xend, _img.cols);
	yend = std::min(yend, _img.rows);

	// if we're up against the top/left bounds, roll the scan forward until we're inside the bounds
	if (xstart < 0)
	{
		int offset = -xstart;
		xstart += offset;
		ystart += offset;
	}
	if (ystart < 0)
	{
		int offset = -ystart;
		xstart += offset;
		ystart += offset;
	}

	// do the scan
	ScanState state;
	for (int x = xstart, y = ystart; x < xend and y < yend; ++x, ++y)
	{
		bool active = test_pixel(x, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(x-res, x, y-res, y));
	}

	// if the pattern is at the edge of the image
	int res = state.process(false);
	if (res > 0)
	{
		int x = xend;
		int y = yend;
		points.push_back(Anchor(x-res, x, y-res, y));
	}
}

std::vector<Anchor> Scanner::t1_scan_rows() const
{
	std::vector<Anchor> points;
	for (int y = _skip; y < _img.rows; y += _skip)
		scan_horizontal(points, y);
	return points;
}

std::vector<Anchor> Scanner::t2_scan_columns(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		int ystart = p.y() - (3 * p.xrange());
		int yend = p.ymax() + (3 * p.xrange());
		scan_vertical(points, p.xavg(), ystart, yend);
	}
	return points;
}

std::vector<Anchor> Scanner::t3_scan_diagonal(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		int xstart = p.x() - (2 * p.yrange());
		int xend = p.xmax() + (2 * p.yrange());
		int ystart = p.y() - p.yrange();
		int yend = p.ymax() + p.yrange();
		scan_diagonal(points, xstart, xend, ystart, yend);
	}
	return points;
}

std::vector<Anchor> Scanner::t4_confirm_scan(const std::vector<Anchor>& candidates) const
{
	// because we have a lot of weird crap going on in the center of the image,
	// do one more scan of our (theoretical) anchor points.
	// this shouldn't be an issue for real anchors, but pretenders might get filtered out.
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		std::vector<Anchor> confirms;
		int xstart = p.x() - p.xrange();
		int xend = p.xmax() + p.xrange();
		if (!scan_horizontal(confirms, p.yavg(), xstart, xend))
			continue;

		int ystart = p.y() - p.yrange();
		int yend = p.ymax() + p.yrange();
		if (!scan_vertical(confirms, p.xavg(), ystart, yend))
			continue;

		Anchor merged(p);
		for (const Anchor& co : confirms)
		{
			if (co.within_merge_distance(p, _mergeCutoff))
				merged.merge(co);
		}
		points.push_back(merged);
	}
	return deduplicate_candidates(points);
}

bool Scanner::sort_top_to_bottom(std::vector<Anchor>& candidates)
{
	std::sort(candidates.begin(), candidates.end());
	if (candidates.size() < 4)
		return false;

	const Anchor& topLeft = candidates.front();
	std::vector<Anchor>::iterator p1_it = ++candidates.begin();
	std::vector<Anchor>::iterator p2_it = ++std::vector<Anchor>::iterator(p1_it);
	int p1_xoff = ::abs(p1_it->xavg() - topLeft.xavg());
	int p2_xoff = ::abs(p2_it->xavg() - topLeft.xavg());
	if (p2_xoff > p1_xoff)
		std::iter_swap(p1_it, p2_it);
	return true;
}

std::vector<Anchor> Scanner::scan()
{
	// scan horizontal
	std::vector<Anchor> candidates = t1_scan_rows();

	// for all horizontal results, scan vertical
	candidates = t2_scan_columns(candidates);

	// for all horizontal+vertical results, scan diagonal
	candidates = t3_scan_diagonal(candidates);

	// for all horizontal+vertical+diagonal results, do one more sanity check
	candidates = t4_confirm_scan(candidates);

	filter_candidates(candidates);
	sort_top_to_bottom(candidates);
	return candidates;
}

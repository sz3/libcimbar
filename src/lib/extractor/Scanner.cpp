#include "Scanner.h"

#include "ScanState.h"
#include "serialize/format.h"

namespace {
	struct size_sort
	{
		inline bool operator() (const Anchor& an1, const Anchor& an2)
		{
			return an1.size() > an2.size();
		}
	};

	unsigned nextTwo(unsigned v)
	{
		// get next power of 2 + 1
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v = ++v >> 1;
		return v + 1;
	}
}

Scanner::Scanner(const cv::Mat& img, bool dark, int skip)
    : _dark(dark)
    , _skip(skip)
{
	_img = preprocess_image(img);
}

cv::Mat Scanner::preprocess_image(const cv::Mat& img)
{
	unsigned unit = nextTwo((unsigned)(std::min(img.rows, img.cols) * 0.05));

	cv::Mat out;
	if (img.channels() >= 3)
		cv::cvtColor(img, out, cv::COLOR_BGR2GRAY);
	else
		out = img;
	cv::adaptiveThreshold(out, out, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, unit, 0);
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
			if (::abs(m.xavg() - c.xavg()) < 50 and ::abs(m.yavg() - c.yavg()) < 50)
			{
				foundMerge = true;
				m.merge(c);
				break;
			}
		}
		if (!foundMerge)
			merged.push_back(c);
	}
	filter_candidates(merged);
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

void Scanner::scan_horizontal(std::vector<Anchor>& points, int y) const
{
	ScanState state;
	for (int x = 0; x < _img.cols; ++x)
	{
		bool active = test_pixel(x, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(x-res, x-1, y, y));
	}

	// if the pattern is at the edge of the image
	int res = state.process(false);
	if (res > 0)
	{
		int x = _img.cols;
		points.push_back(Anchor(x-res, x-1, y, y));
	}
}

void Scanner::scan_vertical(std::vector<Anchor>& points, int x, int ystart, int yend) const
{
	if (ystart < 0)
		ystart = 0;
	if (yend < 0 or yend > _img.rows)
		yend = _img.rows;

	ScanState state;
	for (int y = ystart; y < yend; ++y)
	{
		bool active = test_pixel(x, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(x, x, y-res, y-1));
	}

	// if the pattern is at the edge of the image
	int res = state.process(false);
	if (res > 0)
	{
		int y = yend;
		points.push_back(Anchor(x, x, y-res, y-1));
	}
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

	return deduplicate_candidates(points);
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

	return deduplicate_candidates(points);
}

std::vector<Anchor> Scanner::t3_scan_diagonal(const std::vector<Anchor>& candidates) const
{
	// confirm
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		int xstart = p.x() - (2 * p.yrange());
		int xend = p.xmax() + (2 * p.yrange());
		int ystart = p.y() - p.yrange();
		int yend = p.ymax() + p.yrange();
		scan_diagonal(points, xstart, xend, ystart, yend);
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

	sort_top_to_bottom(candidates);
	return candidates;
}

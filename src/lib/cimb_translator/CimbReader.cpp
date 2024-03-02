/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbReader.h"

#include "CellDrift.h"
#include "Common.h"
#include "Config.h"
#include "Interleave.h"

#include "bit_file/bitmatrix.h"
#include "chromatic_adaptation/adaptation_transform.h"
#include "chromatic_adaptation/color_correction.h"
#include <opencv2/opencv.hpp>

using namespace cimbar;

namespace {
	cv::Mat kernel()
	{
		static const cv::Mat k = (cv::Mat_<float>(3,3) <<  -0, -1, -0, -1, 4.5, -1, -0, -1, -0);
		return k;
	}

	template <typename MAT>
	void sharpenSymbolGrid(const MAT& img, cv::Mat& out)
	{
		cv::filter2D(img, out, -1, kernel());
	}

	template <typename MAT>
	bitbuffer preprocessSymbolGrid(const MAT& img, bool needs_sharpen)
	{
		int blockSize = 5; // default: no preprocessing

		cv::Mat symbols;
		cv::cvtColor(img, symbols, cv::COLOR_RGB2GRAY);
		if (needs_sharpen)
		{
			blockSize = 7;
			sharpenSymbolGrid(symbols, symbols);
		}
		cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, blockSize, 0);

		bitbuffer bb(std::pow(Config::image_size(), 2) / 8);
		bitmatrix::mat_to_bitbuffer(symbols, bb.get_writer());
		return bb;
	}

	void updateMaxColor(std::tuple<float, float, float>& max_color, const cv::Scalar& c)
	{
		std::get<0>(max_color) = std::max(std::get<0>(max_color), static_cast<float>(c[0]));
		std::get<1>(max_color) = std::max(std::get<1>(max_color), static_cast<float>(c[1]));
		std::get<2>(max_color) = std::max(std::get<2>(max_color), static_cast<float>(c[2]));
	}

	std::tuple<float, float, float> calculateWhite(const cv::Mat& img, bool dark)
	{
		std::tuple<float, float, float> bestColor({1, 1, 1});
		if (dark)
		{
			unsigned tl = Config::anchor_size() - 2;
			unsigned br = Config::image_size() - Config::anchor_size() - 2;
			std::array<std::pair<unsigned, unsigned>, 3> anchors = {{ {tl, tl}, {tl, br}, {br, tl} }};
			for (auto [x, y] : anchors)
			{
				cv::Rect crop(x, y, 4, 4);
				cv::Scalar avgColor = cv::mean(img(crop));
				updateMaxColor(bestColor, avgColor);
			}
		}
		else // light
		{
			unsigned tl = (Config::anchor_size() << 1) + 6;
			unsigned br = Config::image_size() - tl - 4;
			std::array<std::pair<unsigned, unsigned>, 4> anchors = {{ {0, tl}, {tl, 0}, {0, br}, {br, 0} }};
			for (auto [x, y] : anchors)
			{
				cv::Rect crop(x, y, 4, 4);
				cv::Scalar avgColor = cv::mean(img(crop));
				updateMaxColor(bestColor, avgColor);
			}
		}
		return bestColor;
	}

	bool simpleColorCorrection(const cv::Mat& img, CimbDecoder& decoder)
	{
		std::tuple<float, float, float> white = calculateWhite(img, Config::dark());
		decoder.update_color_correction(color_correction::get_adaptation_matrix<adaptation_transform::von_kries>(white, {255.0, 255.0, 255.0}));
		return true;
	}
}

CimbReader::CimbReader(const cv::Mat& img, CimbDecoder& decoder, unsigned color_mode, bool needs_sharpen, int color_correction)
	: _image(img)
	, _fountainColorHeader(0U)
	, _cellSize(Config::cell_size() + 2)
	, _positions(Config::cell_spacing(), Config::cells_per_col(), Config::cell_offset(), Config::corner_padding())
	, _decoder(decoder)
	, _good(_image.cols >= Config::image_size() and _image.rows >= Config::image_size())
	, _colorCorrection(color_correction)
	, _colorMode(color_mode)
{
	_grayscale = preprocessSymbolGrid(img, needs_sharpen);
	if (_good and color_correction == 1)
		simpleColorCorrection(_image, decoder);
}

CimbReader::CimbReader(const cv::UMat& img, CimbDecoder& decoder, unsigned color_mode, bool needs_sharpen, int color_correction)
	: CimbReader(img.getMat(cv::ACCESS_READ), decoder, color_mode, needs_sharpen, color_correction)
{
}

unsigned CimbReader::read_color(const PositionData& pos) const
{
	Cell color_cell(_image, pos.x, pos.y, Config::cell_size(), Config::cell_size());
	return _decoder.decode_color(color_cell, _colorMode);
}

unsigned CimbReader::read(PositionData& pos)
{
	if (done())
		return 0;

	// need coordinate, index, and drift from next position
	auto [i, xy, drift, cooldown] = _positions.next();
	int x = xy.first + drift.x();
	int y = xy.second + drift.y();
	bitmatrix cell(_grayscale, _image.cols, _image.rows, x-1, y-1);

	unsigned drift_offset = 0;
	unsigned error_distance;
	unsigned bits = _decoder.decode_symbol(cell, drift_offset, error_distance, cooldown);

	std::pair<int, int> best_drift = CellDrift::driftPairs[drift_offset];
	drift.updateDrift(best_drift.first, best_drift.second);
	_positions.update(i, drift, error_distance, CellDrift::calculate_cooldown(cooldown, drift_offset));

	pos.i = i;
	pos.x = x + best_drift.first;
	pos.y = y + best_drift.second;
	return bits;
}

bool CimbReader::done() const
{
	return !_good or _positions.done();
}

void CimbReader::init_ccm(unsigned color_bits, unsigned interleave_blocks, unsigned interleave_partitions, unsigned fountain_blocks)
{
	if (_colorCorrection != 2)
		return;

	// if no fountain header, we don't attempt color correction
	// we *could* (and used to) sample white pixels in the anchor points, and use the von kries/bradford matrix to generate a primitive CCM
	// but for now we're aiming for something a bit smarter
	if (_fountainColorHeader.id() == 0) // and _decoder.has_no_ccm() ... or something?
		return;

	// TODO: refactor?
	// most logical thing to do is probably to make a get_color_map(), and leave the rest (avg computation, etc) here...?

	// full ccm, using header values as known color index
	// 1. get positions
	// 2. put fountain header into a bitbuffer so we can read decoder.color_bits() bits at a time
	CellPositions::positions_list positions = Interleave::interleave(_positions.positions(), interleave_blocks, interleave_partitions);

	// 3. using expected fountain headers, decode color for each position
	unsigned end = cimbar::Config::capacity(color_bits) * 8 / color_bits;
	unsigned headerStartInterval = cimbar::Config::capacity(_decoder.symbol_bits() + color_bits) * 8 / fountain_blocks / color_bits;
	unsigned headerLen = (_fountainColorHeader.md_size) * 8 / color_bits; // shrink this to md_size-2 to discard the block_id bytes...

	//std::cout << fmt::format("fountain blocks={},capacity={}", fountain_blocks, cimbar::Config::capacity(_decoder.symbol_bits() + color_bits)) << std::endl;
	//std::cout << fmt::format("fountain end={},headerstart={},headerlen={}", end, headerStartInterval, headerLen) << std::endl;

	// get color map
	std::unordered_map<uint16_t, std::tuple<unsigned, unsigned, unsigned, unsigned>> colors;
	bitbuffer buff;
	for (unsigned block = 0; block < end; block+=headerStartInterval)
	{
		// TODO: could just copy/write final 2 bytes after first round?
		buff.copy_to_buffer(reinterpret_cast<const char*>(_fountainColorHeader.data()), _fountainColorHeader.md_size);

		// sample all colors in header
		for (unsigned idx = block, i = 0; idx < block+headerLen; ++idx, i+=color_bits)
		{
			unsigned expected = buff.read(i, color_bits);
			CellPositions::coordinate pos = positions[idx];

			//Cell color_cell(_image, pos.first, pos.second, Config::cell_size(), Config::cell_size());
			//auto col = _decoder.avg_color(color_cell); // could just call cell mean_rgb directly?

			Cell color_cell(_image, pos.first+1, pos.second+1, Config::cell_size()-2, Config::cell_size()-2);
			auto col = color_cell.mean_rgb();

			auto [it, isNew] = colors.try_emplace(expected, std::make_tuple(0, 0, 0, 0)); // count,r,g,b
			std::get<0>(it->second) += 1;
			std::get<1>(it->second) += std::get<0>(col);
			std::get<2>(it->second) += std::get<1>(col);
			std::get<3>(it->second) += std::get<2>(col);
		}

		_fountainColorHeader.increment_block_id();
	}

	// 4. compute avgs
	cv::Mat actual = cv::Mat::ones(0, 3, CV_32F);
	cv::Mat desired = cv::Mat::ones(0, 3, CV_32F);

	for (auto& it : colors)
	{
		unsigned total = std::get<0>(it.second);
		if (total == 0)
			continue;

		std::get<1>(it.second) /= total;
		std::get<2>(it.second) /= total;
		std::get<3>(it.second) /= total;

		cv::Mat arow = (cv::Mat_<float>(1,3) << std::get<1>(it.second), std::get<2>(it.second), std::get<3>(it.second));
		actual.push_back(arow);

		cimbar::RGB cc = _decoder.get_color(it.first, _colorMode);
		cv::Mat drow = (cv::Mat_<float>(1,3) << std::get<0>(cc), std::get<1>(cc), std::get<2>(cc));
		desired.push_back(drow);
	}

	// bail if we don't have enough data...
	if (actual.rows < 4)
		return;

	// 5. sample corners
	{
		std::tuple<float, float, float> white = calculateWhite(_image, Config::dark());
		cv::Mat arow = (cv::Mat_<float>(1,3) << std::get<0>(white), std::get<1>(white), std::get<2>(white));
		actual.push_back(arow);

		cv::Mat drow = (cv::Mat_<float>(1,3) << 255, 255, 255);
		desired.push_back(drow);
	}

	// 6. generate ccm from avgs in #4/5, save in decoder. Success! We hope
	_decoder.update_color_correction(color_correction::get_moore_penrose_lsm(actual, desired));
}

void CimbReader::update_metadata(char* buff, unsigned len)
{
	if (len == 0 and _fountainColorHeader.id() == 0)
		return;

	if (_fountainColorHeader.id() == 0)
		_fountainColorHeader = FountainMetadata(buff, len);

	_fountainColorHeader.increment_block_id(); // we always want to be +1
}

unsigned CimbReader::num_reads() const
{
	return _positions.size();
}

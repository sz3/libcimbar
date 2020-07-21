#pragma once

#include "bit_extractor.h"
#include "bit_file/bitmatrix.h"
#include "intx/int128.hpp"
#include <array>
#include <iostream>
#include <utility>

namespace image_hash
{

class ahash_result
{
public:
	static const int FAST = 5;
	static const int ALL = 9;

	// out from the center.
	// 4 == center.
	// 5, 7, 3, 1 == sides.
	// 8, 0, 2, 6 == corners.
	// mode=FAST will discard the corner checks.
	static constexpr std::array<int, 9> _ORDER = {4, 5, 7, 3, 1, 8, 0, 2, 6};

public:
	class iterator
	{
	public:
		iterator(const ahash_result& hr, unsigned i=0)
		    : _hr(hr)
		    , _i(i)
		{}

		std::pair<int, uint64_t> operator*()
		{
			int idx = _hr._ORDER[_i];
			return {idx, _hr._results[idx]};
		}

		iterator& operator++()
		{
			++_i;
			return *this;
		}

		bool operator!=(const iterator& other) const
		{
			return _i != other._i;
		}

	protected:
		unsigned _i;
		const ahash_result& _hr;
	};

public:
	ahash_result(const intx::uint128& bits, unsigned mode=ALL)
	    : _bits(bits)
	    , _mode(mode)
	{
		if (mode == ALL)
			_results = extract_all();
		else
			_results = extract_fast();
	}

	std::array<uint64_t, 9> extract_all() const
	{
		bit_extractor<intx::uint128, 100> be(_bits);
		return {
			// top row -- top left bit is the start bit (0). bottom right is end bit.
			be.extract(0, 10, 20, 30, 40, 50, 60, 70), // left
			be.extract(1, 11, 21, 31, 41, 51, 61, 71),
			be.extract(2, 12, 22, 32, 42, 52, 62, 72), // right
			// middle row
			be.extract(10, 20, 30, 40, 50, 60, 70, 80),
			be.extract(11, 21, 31, 41, 51, 61, 71, 81),
			be.extract(12, 22, 32, 42, 52, 62, 72, 82),
			// bottom row
			be.extract(20, 30, 40, 50, 60, 70, 80, 90),
			be.extract(21, 31, 41, 51, 61, 71, 81, 91),
			be.extract(22, 32, 42, 52, 62, 72, 82, 92)
		};
	}

	std::array<uint64_t, 9> extract_fast() const
	{
		bit_extractor<intx::uint128, 100> be(_bits);
		// skip the corners
		return {
			0,
			be.extract(1, 11, 21, 31, 41, 51, 61, 71),
			0,
			// middle row
			be.extract(10, 20, 30, 40, 50, 60, 70, 80),
			be.extract(11, 21, 31, 41, 51, 61, 71, 81),
			be.extract(12, 22, 32, 42, 52, 62, 72, 82),
			// bottom row
			0,
			be.extract(21, 31, 41, 51, 61, 71, 81, 91),
			0
		};
	}

	iterator begin() const
	{
		return iterator(*this);
	}

	iterator end() const
	{
		return iterator(*this, _mode);
	}

	uint64_t operator[](unsigned idx) const
	{
		return _results[idx];
	}

	size_t size() const
	{
		return _results.size();
	}

protected:
	int _mode;
	std::array<uint64_t, 9> _results;
	intx::uint128 _bits;
};

}

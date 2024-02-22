/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "AdjacentCellFinder.h"
#include "CellDrift.h"
#include "CellPositions.h"
#include <cstdint>
#include <queue>
#include <set>
#include <tuple>

class FloodDecodePositions
{
public:
	using iter = std::tuple<unsigned, CellPositions::coordinate, CellDrift, uint8_t>;
	// compress these types down to reduce size and cache pressure?
	// i.e. with a struct ...
	using decode_instructions = std::tuple<CellDrift, uint8_t, uint8_t>; // drift, best_prio, cooldown_pos
	using decode_prio = std::tuple<uint16_t, uint8_t>; // index, prio

	class PrioCompare
	{
	public:
		bool operator()(const decode_prio& a, const decode_prio& b) const
		{
			return std::get<1>(a) > std::get<1>(b);
		}
	};

public:
	FloodDecodePositions(int spacing, int dimensions, int offset, int marker_size);

	size_t size() const;
	void reset();

	bool done() const;
	iter next();
	int update(unsigned index, const CellDrift& drift, unsigned error_distance, uint8_t cooldown);

	const CellPositions::positions_list& positions() const;

protected:
	int update_adjacents(const std::array<int,4>& adj, const CellDrift& drift, unsigned error_distance, uint8_t cooldown);

protected:
	unsigned _index;
	unsigned _count;
	std::priority_queue<decode_prio, std::vector<decode_prio>, PrioCompare> _heap;
	std::vector<bool> _remaining;
	std::vector<decode_instructions> _instructions;
	CellPositions::positions_list _positions;
	AdjacentCellFinder _cellFinder;
};


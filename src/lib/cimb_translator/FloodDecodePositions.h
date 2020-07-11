#pragma once

#include "AdjacentCellFinder.h"
#include "CellDrift.h"
#include "CellPositions.h"
#include <queue>
#include <set>
#include <tuple>

class FloodDecodePositions
{
public:
	using iter = std::tuple<unsigned, CellPositions::coordinate, CellDrift>;
	using decode_instructions = std::tuple<unsigned, CellDrift, unsigned>;

	class InstructionsCompare
	{
	public:
		bool operator()(const decode_instructions& a, const decode_instructions& b) const
		{
			return std::get<2>(a) > std::get<2>(b);
		}
	};

public:
	FloodDecodePositions(int spacing, int dimensions, int offset, int marker_size);

	size_t count() const;
	void reset();

	bool done() const;
	iter next();
	int update(unsigned index, const CellDrift& drift, unsigned error_distance);

protected:
	unsigned _index;
	std::priority_queue<decode_instructions, std::vector<decode_instructions>, InstructionsCompare> _heap;
	std::set<unsigned> _remaining;
	CellPositions::positions_list _positions;
	AdjacentCellFinder _cellFinder;
};

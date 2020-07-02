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

inline FloodDecodePositions::FloodDecodePositions(int spacing, int dimensions, int offset, int marker_size)
    : _positions(CellPositions::compute(spacing, dimensions, offset, marker_size, 0))
    , _cellFinder(_positions, dimensions, marker_size)
{
	reset();
}

inline size_t FloodDecodePositions::count() const
{
	return _positions.size();
}

inline void FloodDecodePositions::reset()
{
	_index = 0;
	_remaining.clear();
	for (int i = 0; i < _positions.size(); ++i)
		_remaining.insert(i);

	// seed
	_heap.push({0, CellDrift(), 0});
}

inline bool FloodDecodePositions::done() const
{
	return _remaining.empty();
}

inline FloodDecodePositions::iter FloodDecodePositions::next()
{
	while (!_heap.empty())
	{
		auto [i, drift, _] = _heap.top();
		_heap.pop();
		if (_remaining.erase(i))
		    return {i, _positions[i], drift};
	}

	return {0, {0, 0}, CellDrift()};
}

inline int FloodDecodePositions::update(unsigned index, const CellDrift& drift, unsigned error_distance)
{
	std::array<int,4> adj = _cellFinder.find(index);
	for (int next : adj)
	{
		if (next < 0)
			continue;
		if (_remaining.find(next) == _remaining.end())
			continue;
		_heap.push({next, drift, error_distance});
	}
	return 0;
}

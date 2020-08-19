/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "FloodDecodePositions.h"

FloodDecodePositions::FloodDecodePositions(int spacing, int dimensions, int offset, int marker_size)
    : _positions(CellPositions::compute(spacing, dimensions, offset, marker_size, 0))
    , _cellFinder(_positions, dimensions, marker_size)
{
	reset();
}

size_t FloodDecodePositions::size() const
{
	return _positions.size();
}

void FloodDecodePositions::reset()
{
	_index = 0;
	_count = 0;
	_remaining.clear();
	for (int i = 0; i < _positions.size(); ++i)
	{
		_remaining.push_back(true);
		_instructions.push_back({CellDrift(), ~0UL});
	}

	// seed
	int smallRowLen = _cellFinder.dimensions() - (2*_cellFinder.marker_size()) - 1;
	int lastElem = _positions.size()-1;
	_heap.push({0, 0});
	_heap.push({smallRowLen, 0});
	_heap.push({lastElem, 0});
	_heap.push({lastElem-smallRowLen, 0});
}

bool FloodDecodePositions::done() const
{
	return _count == size();
}

FloodDecodePositions::iter FloodDecodePositions::next()
{
	while (!_heap.empty())
	{
		auto [i, _] = _heap.top();
		_heap.pop();

		std::vector<bool>::reference needsDecode = _remaining[i];
		if (!needsDecode)
		    continue;

		needsDecode = false;
		++_count;
		auto [drift, __] = _instructions[i];
		return {i, _positions[i], drift};
	}

	return {0, {0, 0}, CellDrift()};
}

int FloodDecodePositions::update(unsigned index, const CellDrift& drift, unsigned error_distance)
{
	std::array<int,4> adj = _cellFinder.find(index);
	for (int next : adj)
	{
		if (next < 0 or !_remaining[next])
			continue;
		decode_instructions& di = _instructions[next];
		if (di.second <= error_distance)
			continue;
		di = {drift, error_distance};
		_heap.push({next, error_distance});
	}
	return 0;
}


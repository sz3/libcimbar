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
	for (unsigned i = 0; i < _positions.size(); ++i)
	{
		_remaining.push_back(true);
		_instructions.push_back({CellDrift(), 0xFE, 0xFE});
	}

	// seed
	uint16_t smallRowLen = _cellFinder.dimensions() - (2*_cellFinder.marker_size());
	uint16_t lastElem = _positions.size()-1;
	_heap.push({0, 0});
	_heap.push({smallRowLen-1, 0});
	_heap.push({lastElem, 0});
	_heap.push({lastElem-(smallRowLen-1), 0});

	// add more seed corners?
	uint16_t betweenMarkerBlock = smallRowLen * _cellFinder.marker_size();
	_heap.push({betweenMarkerBlock, 1});
	_heap.push({betweenMarkerBlock+_cellFinder.dimensions()-1, 1});
	_heap.push({lastElem-betweenMarkerBlock, 1});
	_heap.push({lastElem-(betweenMarkerBlock+_cellFinder.dimensions()-1), 1});
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
		auto [drift, __, cooldown] = _instructions[i];
		return {i, _positions[i], drift, cooldown};
	}

	return {0, {0, 0}, CellDrift(), 0xFF};
}

int FloodDecodePositions::update(unsigned index, const CellDrift& drift, unsigned error_distance, uint8_t cooldown)
{
	std::array<int,4> adj = _cellFinder.find(index);
	for (int next : adj)
	{
		if (next < 0 or !_remaining[next])
			continue;
		decode_instructions& di = _instructions[next];
		if (std::get<1>(di) <= error_distance)
			continue;
		di = {drift, error_distance, cooldown};
		_heap.push({next, error_distance});
	}
	return 0;
}


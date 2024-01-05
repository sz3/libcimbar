/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "FloodDecodePositions.h"
#include <iostream>

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

int FloodDecodePositions::update_adjacents(const std::array<int,4>& adj, const CellDrift& drift, unsigned error_distance, uint8_t cooldown)
{
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

int FloodDecodePositions::update(unsigned index, const CellDrift& drift, unsigned error_distance, uint8_t cooldown)
{
	std::array<int,4> adj = _cellFinder.find(index);
	update_adjacents(adj, drift, error_distance, cooldown);

	auto& [_, prev_error, prev_cooldown] = _instructions[index];
	// in the case where we have consecutive high confidence cells with no drift changes,
	// it's safe(ish) to aggressively queue a few more cells
	if (prev_error < 3 and error_distance < 3 and prev_cooldown == 4 and cooldown == 4)
	{
		unsigned rr = 0;
		unsigned ll = 1;
		unsigned dd = 2;
		unsigned uu = 3;

		int rridx = adj[rr];
		int llidx = adj[ll];
		if (rridx >= 0 and llidx >= 0)
		{
			std::array<int,4> horizon = {-1, -1, -1, -1};
			horizon[0] = _cellFinder.right(rridx);
			if (horizon[0] >= 0)
				horizon[1] = _cellFinder.right(horizon[0]);
			horizon[2] = _cellFinder.left(llidx);
			if (horizon[2] >= 0)
				horizon[3] = _cellFinder.left(horizon[2]);

			update_adjacents(horizon, drift, error_distance, cooldown);
		}

		int uuidx = adj[uu];
		int ddidx = adj[dd];
		if (uuidx >= 0 and ddidx >= 0)
		{
			std::array<int,4> vert = {-1, -1, -1, -1};
			vert[0] = _cellFinder.top(uuidx);
			if (vert[0] >= 0)
				vert[1] = _cellFinder.top(vert[0]);
			vert[2] = _cellFinder.bottom(ddidx);
			if (vert[2] >= 0)
				vert[3] = _cellFinder.bottom(vert[2]);

			update_adjacents(vert, drift, error_distance, cooldown);
		}
	}

	prev_error = error_distance;
	prev_cooldown = cooldown;
	return 0;
}

const CellPositions::positions_list& FloodDecodePositions::positions() const
{
	return _positions;
}

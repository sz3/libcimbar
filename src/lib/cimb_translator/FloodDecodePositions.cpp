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
		_remaining.push_back(true);

	// seed
	int smallRowLen = _cellFinder.dimensions() - (2*_cellFinder.marker_size()) - 1;
	int lastElem = _positions.size()-1;
	_heap.push({0, CellDrift(), 0});
	_heap.push({smallRowLen, CellDrift(), 0});
	_heap.push({lastElem, CellDrift(), 0});
	_heap.push({lastElem-smallRowLen, CellDrift(), 0});
}

bool FloodDecodePositions::done() const
{
	return _count == size();
}

FloodDecodePositions::iter FloodDecodePositions::next()
{
	while (!_heap.empty())
	{
		auto [i, drift, _] = _heap.top();
		_heap.pop();

		std::vector<bool>::reference needsDecode = _remaining[i];
		if (!needsDecode)
		    continue;

		needsDecode = false;
		++_count;
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
		_heap.push({next, drift, error_distance});
	}
	return 0;
}


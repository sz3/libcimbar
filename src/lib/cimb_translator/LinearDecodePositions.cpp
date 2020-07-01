#include "LinearDecodePositions.h"

#include <vector>
#include <utility>

LinearDecodePositions::LinearDecodePositions(int spacing, int dimensions, int offset, int marker_size)
    : _positions(CellPositions::compute(spacing, dimensions, offset, marker_size, 0))
    , _drift()
{
	reset();
}

size_t LinearDecodePositions::count() const
{
	return _positions.size();
}

void LinearDecodePositions::reset()
{
	_index = 0;
	_drift = CellDrift();
}

bool LinearDecodePositions::done() const
{
	return _index >= _positions.size();
}

LinearDecodePositions::iter LinearDecodePositions::next()
{
	unsigned i = _index++;
	return {i, _positions[i], _drift};
}

int LinearDecodePositions::update(unsigned index, const CellDrift& drift, unsigned error_distance)
{
	_drift = drift;
	return 0;
}

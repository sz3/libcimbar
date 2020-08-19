/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CellDrift.h"
#include "CellPositions.h"
#include <tuple>

class LinearDecodePositions
{
public:
	using iter = std::tuple<unsigned, CellPositions::coordinate, CellDrift>;

public:
	LinearDecodePositions(int spacing, int dimensions, int offset, int marker_size);

	size_t count() const;
	void reset();

	bool done() const;
	iter next();
	int update(unsigned index, const CellDrift& drift, unsigned error_distance);

protected:
	unsigned _index;
	CellPositions::positions_list _positions;
	CellDrift _drift;
};

inline LinearDecodePositions::LinearDecodePositions(int spacing, int dimensions, int offset, int marker_size)
    : _positions(CellPositions::compute(spacing, dimensions, offset, marker_size, 0))
    , _drift()
{
	reset();
}

inline size_t LinearDecodePositions::count() const
{
	return _positions.size();
}

inline void LinearDecodePositions::reset()
{
	_index = 0;
	_drift = CellDrift();
}

inline bool LinearDecodePositions::done() const
{
	return _index >= _positions.size();
}

inline LinearDecodePositions::iter LinearDecodePositions::next()
{
	unsigned i = _index++;
	return {i, _positions[i], _drift};
}

inline int LinearDecodePositions::update(unsigned index, const CellDrift& drift, unsigned error_distance)
{
	_drift = drift;
	return 0;
}

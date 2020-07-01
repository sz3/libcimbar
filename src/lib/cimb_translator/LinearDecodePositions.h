#pragma once

#include "CellDrift.h"
#include "CellPositions.h"
#include <tuple>
#include <vector>
#include <utility>

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

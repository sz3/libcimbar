#pragma once

#include <vector>
#include <utility>

class CellPosition
{
public:
	using coordinate = std::pair<int, int>;
	using positions_list = std::vector<coordinate>;

	static positions_list compute_linear(int spacing, int dimensions, int offset, int marker_size);
	static positions_list compute(int spacing, int dimensions, int offset, int marker_size, int interleave_blocks=0);

public:
	CellPosition(int spacing, int dimensions, int offset, int marker_size, int interleave_blocks=0);

	size_t count() const;
	void reset();
	bool done() const;
	const coordinate& next();

protected:
	unsigned _index;
	positions_list _positions;
};

#pragma once

#include <vector>
#include <utility>

class CellPositions
{
public:
	using coordinate = std::pair<int, int>;
	using positions_list = std::vector<coordinate>;

	static positions_list compute_linear(int spacing, int dimensions, int offset, int marker_size);
	static positions_list compute(int spacing, int dimensions, int offset, int marker_size, int interleave_blocks=0, int interleave_partitions=1);

public:
	CellPositions(int spacing, int dimensions, int offset, int marker_size, int interleave_blocks=0, int interleave_partitions=1);

	unsigned index() const;
	size_t count() const;
	void reset();
	bool done() const;
	const coordinate& next();

	const positions_list& positions() const;

protected:
	unsigned _index;
	positions_list _positions;
};

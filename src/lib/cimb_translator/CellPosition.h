#pragma once

#include <vector>
#include <utility>

class CellPosition
{
public:
	using coordinate = std::pair<int, int>;
	using positions_list = std::vector<coordinate>;

	static positions_list compute(int spacing, int dimensions, int offset, int marker_size);

public:
	CellPosition(int spacing, int dimensions, int offset, int marker_size);

	void reset();
	bool done() const;
	const coordinate& next();

protected:
	unsigned _index;
	positions_list _positions;
};

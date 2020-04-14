#pragma once

#include "ICimbWriter.h"

class CimbWriter : public ICimbWriter
{
public:
	CimbWriter();

	bool write(unsigned bits, unsigned bits_per_op);

protected:
};

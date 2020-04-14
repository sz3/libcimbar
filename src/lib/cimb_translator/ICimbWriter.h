#pragma once

class ICimbWriter
{
public:
	virtual ~ICimbWriter() {}

	virtual bool write(unsigned bits, unsigned bits_per_op) = 0;
};

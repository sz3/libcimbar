#pragma once

class ICimbWriter
{
public:
	virtual ~ICimbWriter() {}

	virtual bool write(unsigned bits) = 0;
};

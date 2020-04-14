#pragma once

#include <string>

class ICimbWriter
{
public:
	virtual ~ICimbWriter() {}

	virtual bool write(unsigned bits) = 0;
	virtual bool save(std::string filename) const = 0;
};

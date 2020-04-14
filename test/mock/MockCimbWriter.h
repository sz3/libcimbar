#pragma once

#include "cimb_translator/ICimbWriter.h"

#include <string>

class MockCimbWriter : public ICimbWriter
{
public:
	MockCimbWriter() {}

	bool write(unsigned bits)
	{
		return true;
	}

	bool save(std::string filename) const
	{
		return true;
	}
};


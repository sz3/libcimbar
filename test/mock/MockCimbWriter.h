#pragma once

#include "cimb_translator/ICimbWriter.h"

#include <map>
#include <memory>
#include <string>

class MockCimbWriter : public ICimbWriter
{
public:
	MockCimbWriter() {}

	bool write(unsigned bits)
	{
		return true;
	}
};


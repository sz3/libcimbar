
#include "serialize/format.h"
#include <string>

namespace TestCimbar
{
	static inline std::string getSample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}

	static inline std::string getTileDir(unsigned symbol_bits)
	{
		return fmt::format("{}/bitmap/{}", LIBCIMBAR_PROJECT_ROOT, symbol_bits);
	}
}



#include "serialize/format.h"
#include <string>

namespace TestCimbar
{
	inline std::string getSample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}

	inline std::string getProjectDir()
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT);
	}
}


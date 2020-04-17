/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <sstream>

namespace turbo {
namespace str
{
	template <class Iter>
	inline std::string join(const Iter& start, const Iter& end, char delim=' ')
	{
		std::stringstream ss;
		Iter it = start;
		if (it != end)
			ss << *it++;
		for (; it != end; ++it)
			ss << delim << *it;
		return ss.str();
	}

	template <class Type>
	inline std::string join(const Type& container, char delim=' ')
	{
		return join(container.begin(), container.end(), delim);
	}
}
}// namespace turbo

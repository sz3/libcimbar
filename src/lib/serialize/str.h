/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace turbo {
namespace str
{
	template <typename Type>
	inline std::string str(const Type& val)
	{
		std::stringstream ss;
		ss << val;
		return ss.str();
	}

	template <>
	inline std::string str<bool>(const bool& val)
	{
		return val? "true" : "false";
	}

	template <typename Integer>
	inline std::string hexStr(const Integer& number)
	{
		std::stringstream ss;
		ss << std::hex << number;
		return ss.str();
	}

	template <typename T>
	inline bool fromStr(T& var, const std::string& str)
	{
		if (str.empty())
			return false;
		std::stringstream ss(str);
		ss >> var;
		return !!ss;
	}

	template <>
	inline bool fromStr(std::string& var, const std::string& str)
	{
		var = str;
		return true;
	}

	inline std::vector<std::string> split(const std::string& input, char delim=' ', bool ignoreEmpty=false)
	{
		std::vector<std::string> tokens;

		size_t pos = 0;
		size_t nextPos = 0;
		while ((nextPos = input.find(delim, pos)) != std::string::npos)
		{
			std::string tok = input.substr(pos,nextPos-pos);
			if (!ignoreEmpty || !tok.empty())
				tokens.push_back(tok);
			pos = nextPos+1;
		}

		if (!input.empty())
		{
			std::string tok = input.substr(pos);
			if (!ignoreEmpty || !tok.empty())
				tokens.push_back(tok);
		}

		return tokens;
	}

	template <typename Container>
	inline Container sort(Container input)
	{
		std::sort(input.begin(), input.end());
		return input;
	}
}
}// namespace turbo

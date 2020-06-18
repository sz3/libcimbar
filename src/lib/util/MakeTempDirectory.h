#pragma once

#include <cstdio>
#include <experimental/filesystem>
#include <string>

class MakeTempDirectory
{
public:
	MakeTempDirectory()
	{
		_path = std::tmpnam(nullptr);
		std::experimental::filesystem::create_directory(_path);
	}

	~MakeTempDirectory()
	{
		std::error_code ec;
		std::experimental::filesystem::remove_all(_path, ec);
	}

	std::experimental::filesystem::path path() const
	{
		return _path;
	}

protected:
	std::experimental::filesystem::path _path;
};

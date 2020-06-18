#pragma once

#include <cstdio>
#include <experimental/filesystem>
#include <string>

class MakeTempDirectory
{
public:
	MakeTempDirectory(bool cleanup=true)
		: _cleanup(cleanup)
	{
		_path = std::tmpnam(nullptr);
		std::experimental::filesystem::create_directory(_path);
	}

	~MakeTempDirectory()
	{
		if (_cleanup)
		{
			std::error_code ec;
			std::experimental::filesystem::remove_all(_path, ec);
		}
	}

	std::experimental::filesystem::path path() const
	{
		return _path;
	}

protected:
	bool _cleanup;
	std::experimental::filesystem::path _path;
};

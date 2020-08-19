/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <array>
#include <cstdio>
#include <iostream>
#include <string>

class File
{
public:
	File(std::string filename, bool write=false)
	{
		_fp = fopen(filename.c_str(), write? "wb" : "rb");
	}

	std::string read_all()
	{
		std::string res;
		std::array<char, 8192> buffer;
		while (1)
		{
			unsigned bytesRead = read(buffer.data(), buffer.size());
			if (!bytesRead)
				break;
			res += std::string(buffer.data(), bytesRead);
		}
		return res;
	}

	unsigned read(char* buffer, unsigned length)
	{
		if (!good())
			return 0;

		unsigned res = fread(buffer, sizeof(char), length, _fp);
		if (res != length)
			close();
		return res;
	}

	virtual unsigned write(const char* buffer, unsigned length)
	{
		if (!good())
			return 0;

		unsigned res = fwrite(buffer, sizeof(char), length, _fp);
		if (res != length)
			close();
		return res;
	}

	virtual ~File()
	{
		close();
	}

	bool close()
	{
		if (_fp != NULL)
		{
			fclose(_fp);
			_fp = NULL;
			return true;
		}
		return false;
	}

	bool good() const
	{
		return _fp != NULL;
	}

protected:
	FILE* _fp;
};

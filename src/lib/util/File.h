#pragma once

#include <cstdio>

class File
{
public:
	File(std::string filename, bool write=false)
	{
		_fp = fopen(filename.c_str(), write? "wb" : "rb");
		_good = true;
	}

	unsigned read(char* buffer, unsigned bytes)
	{
		unsigned res = fread(buffer, sizeof(char), bytes, _fp);
		if (res != bytes)
			_good = false;
		return res;
	}

	unsigned write(char* buffer, unsigned bytes)
	{
		unsigned res = fwrite(buffer, sizeof(char), bytes, _fp);
		if (res != bytes)
			_good = false;
		return res;
	}

	~File()
	{
		if (_fp != NULL)
			fclose(_fp);
	}

	bool good() const
	{
		return _good;
	}

protected:
	FILE* _fp;
	bool _good;
};

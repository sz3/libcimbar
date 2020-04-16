#pragma once

#include <cstdio>

class File
{
public:
	File(std::string filename, bool write=false)
	{
		_fp = fopen(filename.c_str(), write? "wb" : "rb");
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

	unsigned write(const char* buffer, unsigned length)
	{
		if (!good())
			return 0;

		unsigned res = fwrite(buffer, sizeof(char), length, _fp);
		if (res != length)
			close();
		return res;
	}

	~File()
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

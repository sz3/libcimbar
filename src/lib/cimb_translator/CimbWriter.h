#pragma once

#include "ICimbWriter.h"

#include "CellPosition.h"
#include "CimbEncoder.h"

class CimbWriter : public ICimbWriter
{
public:
	CimbWriter(unsigned size=1024);
	void initialize(unsigned size);

	bool write(unsigned bits);
	bool save(std::string filename) const;

protected:
	cv::Mat _image;
	CellPosition _position;
	CimbEncoder _encoder;
};

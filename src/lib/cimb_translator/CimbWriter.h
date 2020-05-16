#pragma once

#include "ICimbWriter.h"

#include "CellPosition.h"
#include "CimbEncoder.h"

class CimbWriter : public ICimbWriter
{
public:
	CimbWriter(bool dark=true, unsigned size=1024);

	bool write(unsigned bits);
	bool done() const;

	bool save(std::string filename) const;
	cv::Mat image() const;

protected:
	cv::Mat _image;
	CellPosition _position;
	CimbEncoder _encoder;
};

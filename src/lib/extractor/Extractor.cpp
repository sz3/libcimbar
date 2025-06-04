/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Extractor.h"

#include "cimb_translator/Config.h"

Extractor::Extractor(cimbar::vec_xy image_size, unsigned anchor_size)
	: _imageSize({
		image_size.width()? image_size.width() : cimbar::Config::image_size_x(),
		image_size.height()? image_size.height() : cimbar::Config::image_size_y()})
	, _anchorSize(anchor_size? anchor_size : cimbar::Config::anchor_size())
{
}

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_zstd_js.h"

#include "compression/zstd_decompressor.h"

#include <memory>
#include <sstream>

namespace {
	// maybe a map eventually
	std::unique_ptr<cimbar::zstd_decompressor<std::stringstream>> _dec;

}

// zstd_decompressor constructor, reads skippable frame at start? (needs to for ofstream case)
// if we make _dec a map, this'll also take the uin32_t id we got from the decoder sink, probably
int cimbarz_init_decompress(unsigned char* buffer, unsigned size)
{
	if (_dec)
		_dec.reset();

	_dec = std::make_unique<cimbar::zstd_decompressor<std::stringstream>>();
	if (!_dec)
		return -1;
	_dec->init_decompress(reinterpret_cast<char*>(buffer), size);
	return 0;
}

int cimbarz_get_bufsize()
{
	return ZSTD_DStreamOutSize();
}

// "decompress_chunk()/write()"
int cimbarz_decompress_read(unsigned char* buffer, unsigned size)
{
	if (!_dec)
		return -1;
	if (!_dec->good())
		return -2;

	_dec->str(std::string());
	_dec->write_once();
	std::string temp = _dec->str();
	if (size > temp.size())
		size = temp.size();
	std::copy(temp.data(), temp.data()+size, buffer);
	return size;
}

/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#ifndef CIMBAR_RECV_JS_API_H
#define CIMBAR_RECV_JS_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned get_report(unsigned char* buff, unsigned maxlen);
int do_decode(unsigned char* rgba_image_data, int width, int height);

// imgsize=width*height*channels. Stores results in `bufspace`
int fountain_get_bufsize();
int scan_extract_decode(unsigned char* imgdata, unsigned imgw, unsigned imgh, int channels, unsigned char* bufspace, unsigned bufsize);

// returns id of final file (can be used to get size of `finish_copy`'s buffer) if complete, 0 if success, negative on error
int64_t fountain_decode(unsigned char* buffer, unsigned size);

// get filesize, filename from id
int fountain_get_filesize(uint32_t id);
int fountain_get_filename(uint32_t id, char* buf, unsigned size);

// if fountain_decode returned a >0 value, call this to retrieve the reassembled file
// bouth fountain_*() calls should be from the same js webworker/thread
int fountain_finish_copy(uint32_t id, unsigned char* buffer, unsigned size);

int configure_decode(unsigned color_bits, int mode_val);

#ifdef __cplusplus
}
#endif

#endif // CIMBAR_RECV_JS_API_H

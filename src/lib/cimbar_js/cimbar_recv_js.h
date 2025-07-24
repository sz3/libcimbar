/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#ifndef CIMBAR_RECV_JS_API_H
#define CIMBAR_RECV_JS_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned cimbard_get_report(unsigned char* buff, unsigned maxlen);
unsigned cimbard_get_debug(unsigned char* buff, unsigned maxlen);

// imgsize=width*height*channels for rgba. Other formats are weirder.
// output of scan is stored in `bufspace`
int cimbard_get_bufsize();
int cimbard_scan_extract_decode(unsigned char* imgdata, unsigned imgw, unsigned imgh, int format, unsigned char* bufspace, unsigned bufsize);

// returns id of final file (can be used to get size of `finish_copy`'s buffer) if complete, 0 if success, negative on error
// persists state, the return value (if >0) corresponds to a uint32_t id
int64_t cimbard_fountain_decode(unsigned char* buffer, unsigned size);

// get filesize from id
int cimbard_get_filesize(uint32_t id);

// if fountain_decode returned a >0 value, call this to retrieve the reassembled file
// wherever a uint32_t id is passed, it should be on the same js thread
// ... or at least in the same js shared memory...
// as the fountain_decode() call
int cimbard_finish_copy(uint32_t id, unsigned char* buffer, unsigned size);

int cimbard_configure_decode(unsigned color_bits, int mode_val);

#ifdef __cplusplus
}
#endif

#endif // CIMBAR_RECV_JS_API_H

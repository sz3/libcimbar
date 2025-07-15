/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#ifndef CIMBAR_ZSTD_JS_API_H
#define CIMBAR_ZSTD_JS_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int cimbarz_init_decompress(unsigned char* buffer, unsigned size);
int cimbarz_get_filename(char* buffer, unsigned size);
int cimbarz_get_bufsize();
int cimbarz_decompress_read(unsigned char* buffer, unsigned size);

#ifdef __cplusplus
}
#endif

#endif // CIMBAR_ZSTD_JS_API_H

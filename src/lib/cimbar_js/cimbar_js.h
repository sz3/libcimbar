/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#ifndef CIMBAR_JS_API_H
#define CIMBAR_JS_API_H

#ifdef __cplusplus
extern "C" {
#endif

int initialize_GL(int width, int height);
int render();
int next_frame();
int encode(unsigned char* buffer, unsigned size);
int configure(unsigned color_bits, unsigned ecc, int compression);

#ifdef __cplusplus
}
#endif

#endif // CIMBAR_JS_API_H

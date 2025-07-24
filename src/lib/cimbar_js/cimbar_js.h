/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#ifndef CIMBAR_JS_API_H
#define CIMBAR_JS_API_H

#ifdef __cplusplus
extern "C" {
#endif

int cimbare_init_window(int width, int height);
int cimbare_render();
int cimbare_next_frame();
int cimbare_encode(unsigned char* buffer, unsigned size, int encode_id);  // encode_id == -1 -> auto-increment
int cimbare_configure(unsigned color_bits, unsigned ecc, int compression, bool legacy_mode);
float cimbare_get_aspect_ratio();

// internal usage
bool cimbare_auto_scale_window();
int cimbare_get_frame_buff(unsigned char** buff);

#ifdef __cplusplus
}
#endif

#endif // CIMBAR_JS_API_H

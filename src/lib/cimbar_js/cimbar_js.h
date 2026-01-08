/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#ifndef CIMBAR_JS_API_H
#define CIMBAR_JS_API_H

#ifdef __cplusplus
extern "C" {
#endif

int cimbare_init_window(int width, int height);
int cimbare_rotate_window(bool rotate);
int cimbare_render();
int cimbare_next_frame(bool color_balance=false);
int cimbare_init_encode(const char* filename, unsigned fnsize, int encode_id);
int cimbare_encode_bufsize();
int cimbare_encode(const unsigned char* buffer, unsigned size);
int cimbare_configure(int mode_val, int compression);
float cimbare_get_aspect_ratio();

// internal usage
bool cimbare_auto_scale_window();
int cimbare_get_frame_buff(unsigned char** buff);

#ifdef __cplusplus
}
#endif

#endif // CIMBAR_JS_API_H

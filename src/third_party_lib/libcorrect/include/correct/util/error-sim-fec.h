#include "correct/util/error-sim.h"

#include <fec.h>

void conv_fec27_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
void conv_fec29_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
void conv_fec39_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
void conv_fec615_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);

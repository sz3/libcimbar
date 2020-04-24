#include "correct/convolutional/convolutional.h"
#include "correct/convolutional/sse/lookup.h"
// BIG HEAPING TODO sort out the include mess
#include "correct-sse.h"
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


struct correct_convolutional_sse {
    correct_convolutional base_conv;
    oct_lookup_t oct_lookup;
};

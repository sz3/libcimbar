#pragma once

#if defined(__clang__) || defined(__GNUC__)
    #define CIMBAR_ALWAYS_INLINE __attribute__((always_inline))
    #define CIMBAR_FLATTEN       __attribute__((flatten))
#else
    #define CIMBAR_ALWAYS_INLINE
    #define CIMBAR_FLATTEN
#endif


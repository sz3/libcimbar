#ifdef __GNUC__
#define HAVE_BUILTINS
#endif


#ifdef HAVE_BUILTINS
#define popcount __builtin_popcount
#define prefetch __builtin_prefetch
#else

static inline int popcount(int x) {
    /* taken from the helpful http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel */
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    return ((x + (x >> 4) & 0x0f0f0f0f) * 0x01010101) >> 24;
}

static inline void prefetch(void *x) {}

#endif

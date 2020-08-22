set(SRCFILES huf_decompress.c  zstd_ddict.c  zstd_ddict.h  zstd_decompress_block.c  zstd_decompress_block.h  zstd_decompress.c  zstd_decompress_internal.h)
add_library(zstd-decompress OBJECT ${SRCFILES})

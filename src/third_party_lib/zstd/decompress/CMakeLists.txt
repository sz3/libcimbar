set(SRCFILES huf_decompress.c  zstd_ddict.h             zstd_decompress_block.h  zstd_decompress_internal.h
huf_decompress_amd64.S  zstd_ddict.c      zstd_decompress_block.c  zstd_decompress.c)
add_library(zstd-decompress OBJECT ${SRCFILES})

target_compile_options(zstd-decompress PUBLIC "-DZSTD_DISABLE_ASM")

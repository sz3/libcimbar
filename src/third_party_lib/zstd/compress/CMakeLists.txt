set(SRCFILES clevels.h       huf_compress.c            zstd_compress_sequences.c   zstd_double_fast.c  zstd_lazy.h         zstdmt_compress.h
 zstd_compress.c           zstd_compress_sequences.h   zstd_double_fast.h  zstd_ldm.c          zstd_opt.c
fse_compress.c  zstd_compress_internal.h  zstd_compress_superblock.c  zstd_fast.c         zstd_ldm_geartab.h  zstd_opt.h
hist.c          zstd_compress_literals.c  zstd_compress_superblock.h  zstd_fast.h         zstd_ldm.h
hist.h          zstd_compress_literals.h  zstd_cwksp.h                zstd_lazy.c         zstdmt_compress.c)
add_library(zstd-compress OBJECT ${SRCFILES})

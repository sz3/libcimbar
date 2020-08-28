set(SRCFILES huf_compress.c            zstd_compress_literals.h    zstd_compress_superblock.h  zstd_fast.c  zstd_ldm.c         zstd_opt.c
fse_compress.c  zstd_compress.c           zstd_compress_sequences.c   zstd_cwksp.h                zstd_fast.h  zstd_ldm.h         zstd_opt.h
hist.c          zstd_compress_internal.h  zstd_compress_sequences.h   zstd_double_fast.c          zstd_lazy.c  zstdmt_compress.c
hist.h          zstd_compress_literals.c  zstd_compress_superblock.c  zstd_double_fast.h          zstd_lazy.h  zstdmt_compress.h)
add_library(zstd-compress OBJECT ${SRCFILES})

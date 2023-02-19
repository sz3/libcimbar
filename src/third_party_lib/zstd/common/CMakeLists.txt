set(SRCFILES allocations.h     debug.c           error_private.c   fse.h  pool.c                threading.c  xxhash.h       zstd_internal.h
bits.h         compiler.h      debug.h           error_private.h   huf.h  pool.h                threading.h  zstd_common.c  zstd_trace.h
bitstream.h    cpu.h           entropy_common.c  fse_decompress.c  mem.h  portability_macros.h  xxhash.c     zstd_deps.h
)
add_library(zstd-common OBJECT ${SRCFILES})

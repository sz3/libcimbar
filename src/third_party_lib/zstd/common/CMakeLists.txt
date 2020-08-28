set(SRCFILES bitstream.h     compiler.h  debug.c  entropy_common.c  error_private.h   fse.h  mem.h   pool.h       threading.h  xxhash.h       zstd_errors.h
  cpu.h       debug.h  error_private.c   fse_decompress.c  huf.h  pool.c  threading.c  xxhash.c     zstd_common.c  zstd_internal.h)
add_library(zstd-common OBJECT ${SRCFILES})

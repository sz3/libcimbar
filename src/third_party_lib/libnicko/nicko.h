/*
 * Copyright (c) 2018 bindh3x <os@bindh3x.io>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef LIBNICKO_H
#define LIBNICKO_H

#include <stdint.h>

#define NICKO_STRING_MAX 256
#define NICKO_MAGIC_MAX  24

#define LIBNICKO_VERSION_MAJOR  1
#define LIBNICKO_VERSION_MINOR  0
#define LIBNICKO_VERSION_PATCH  0

enum {
  NICKO_TYPE_DIR=1,
  NICKO_TYPE_LINK,
  NICKO_TYPE_SOCKET,
  NICKO_TYPE_PIPE,
  NICKO_TYPE_DEVICE_CHAR,
  NICKO_TYPE_DEVICE_BLOCK,
  NICKO_TYPE_EMPTY,
  NICKO_TYPE_ZIP,
  NICKO_TYPE_RAR,
  NICKO_TYPE_GZ,
  NICKO_TYPE_BZ2,
  NICKO_TYPE_ISO,
  NICKO_TYPE_TAR,
  NICKO_TYPE_XZ,
  NICKO_TYPE_Z,
  NICKO_TYPE_RPM,
  NICKO_TYPE_ELF,
  NICKO_TYPE_EXE,
  NICKO_TYPE_MACH_O,
  NICKO_TYPE_PDF,
  NICKO_TYPE_POSTSCRIPT,
  NICKO_TYPE_MP3,
  NICKO_TYPE_FLAC,
  NICKO_TYPE_OGG,
  NICKO_TYPE_MATROSKA,
  NICKO_TYPE_WAV,
  NICKO_TYPE_AVI,
  NICKO_TYPE_PNG,
  NICKO_TYPE_JPG,
  NICKO_TYPE_GIF,
  NICKO_TYPE_ICO,
  NICKO_TYPE_SCRIPT,
  NICKO_TYPE_MBR
};

enum {
  NICKO_GROUP_SYSTEM,   /* System files */
  NICKO_GROUP_BIN,      /* Binaries */
  NICKO_GROUP_ARCHIVE,  /* Archive formats. */
  NICKO_GROUP_AUDIO,    /* Audio */
  NICKO_GROUP_DOCUMENT, /* Document */
  NICKO_GROUP_PM,       /* Package managers */
  NICKO_GROUP_PICTURE,   /* Picture */
  NICKO_GROUP_VIDEO,
  NICKO_GROUP_UNSPECIFIED
};

struct nicko_magic {
  unsigned int type;
  unsigned int group;
  const char *name;
  uint8_t magic[NICKO_MAGIC_MAX];
  size_t size;
  int offset;
};

/**
 * Run nicko on a file.
 *
 * @param filename: file name.
 * @return 0 on success, -1 on error.
 */
int nicko(const char *filename, struct nicko_magic **p);

/**
 * Get group name by group ID.
 *
 * @param group: group ID.
 * @return printable name of the group.
 */
const char *nicko_get_group_name(int group);

#endif /* LIBNICKO_H */



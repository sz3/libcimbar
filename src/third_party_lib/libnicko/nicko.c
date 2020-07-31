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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h> /* memset */

#include <sys/stat.h>
#include <sys/types.h>

#include "nicko.h"

static const char *nicko_groups[] = {
  [NICKO_GROUP_SYSTEM]   = "system",
  [NICKO_GROUP_BIN]      = "binary",
  [NICKO_GROUP_ARCHIVE]  = "archive",
  [NICKO_GROUP_AUDIO]    = "audio",
  [NICKO_GROUP_DOCUMENT] = "document",
  [NICKO_GROUP_PM]       = "package manager file",
  [NICKO_GROUP_PICTURE]   = "picture",
  [NICKO_GROUP_VIDEO]    = "video",
  [NICKO_GROUP_UNSPECIFIED] = "unspecified"

};

static struct nicko_magic list[] = {
  {
	NICKO_TYPE_DIR,
	0,
	"directory",
	{0},
	0,
	0
  },
  {
	NICKO_TYPE_LINK,
	NICKO_GROUP_SYSTEM,
	"link",
	{0},
	0,
	0
  },
  {
	NICKO_TYPE_SOCKET,
	NICKO_GROUP_SYSTEM,
	"socket",
	{0},
	0,
	0
  },
  {
	NICKO_TYPE_PIPE,
	NICKO_GROUP_SYSTEM,
	"pipe",
	{0},
	0,
	0
  },
  {
	NICKO_TYPE_DEVICE_CHAR,
	NICKO_GROUP_SYSTEM,
	"device/character",
	{0},
	0,
	0
  },
  {
	NICKO_TYPE_DEVICE_BLOCK,
	NICKO_GROUP_SYSTEM,
	"device/block",
	{0},
	0,
	0
  },
  {
	NICKO_TYPE_EMPTY,
	NICKO_GROUP_SYSTEM,
	"empty",
	{0},
	0,
	0
  },
  {
	NICKO_TYPE_ZIP,
	NICKO_GROUP_ARCHIVE,
	"zip",
	{0x50, 0x4b, 0x03, 0x04},
	4,
	0
  },
  {
	NICKO_TYPE_ZIP,
	NICKO_GROUP_ARCHIVE,
	"zip",
	{0x50, 0x4B, 0x05, 0x06},
	4,
	0
  },
  {
	NICKO_TYPE_ZIP,
	NICKO_GROUP_ARCHIVE,
	"zip",
	{0x50, 0x4B, 0x07, 0x08},
	4,
	0
  },
  {
	NICKO_TYPE_RAR,
	NICKO_GROUP_ARCHIVE,
	"rar",
	{0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00},
	7,
	0
  }, /* RAR 1.50 */
  {
	NICKO_TYPE_RAR,
	NICKO_GROUP_ARCHIVE,
	"rar",
	{0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00},
	8,
	0
  }, /* RAR 5.0 */
  {
	NICKO_TYPE_GZ,
	NICKO_GROUP_ARCHIVE,
	"gz",
	{0x1F, 0x8B},
	2,
	0
  },
  {
	NICKO_TYPE_BZ2,
	NICKO_GROUP_ARCHIVE,
	"bz2",
	{0x42, 0x5A, 0x68},
	3,
	0
  },
  {
	NICKO_TYPE_ISO,
	NICKO_GROUP_ARCHIVE,
	"iso",
	{0x43, 0x44, 0x30, 0x30, 0x31},
	5,
	0
  },
  {
	NICKO_TYPE_TAR,
	NICKO_GROUP_ARCHIVE,
	"tar",
	{0x75, 0x73, 0x74, 0x61, 0x72, 0x00, 0x30, 0x30},
	8,
	0
  },
  {
	NICKO_TYPE_TAR,
	NICKO_GROUP_ARCHIVE,
	"tar",
	{0x75, 0x73, 0x74, 0x61, 0x72, 0x20, 0x20, 0x00},
	8,
	0
  },
  {
	NICKO_TYPE_XZ,
	NICKO_GROUP_ARCHIVE,
	"xz",
	{0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00, 0x00},
	7,
	0
  },
  {
	NICKO_TYPE_Z,
	NICKO_GROUP_ARCHIVE,
	"z",
	{0x1F, 0xA0},
	2,
	0
  },
  {
	NICKO_TYPE_RPM,
	NICKO_GROUP_PM,
	"rpm",
	{0xed, 0xab, 0xee, 0xdb},
	4,
	0
  },
  {
	NICKO_TYPE_ELF,
	NICKO_GROUP_BIN,
	"elf",
	{0x7f, 0x45, 0x4c, 0x46},
	4,
	0
  },
  {
	NICKO_TYPE_EXE,
	NICKO_GROUP_BIN,
	"exe",
	{0x4D, 0x5A},
	2,
	0
  },
  {
	NICKO_TYPE_MACH_O,
	NICKO_GROUP_BIN,
	"mach-o",
	{0xCE, 0xFA, 0xED, 0xFE},
	4,
	0
  }, /* Mach-O 32-bit */
  {
	NICKO_TYPE_MACH_O,
	NICKO_GROUP_BIN,
	"mach-o",
	{0xCF, 0xFA, 0xED, 0xFE},
	4,
	0
  }, /* Mach-O 64-bit */
  {
	NICKO_TYPE_PDF,
	NICKO_GROUP_DOCUMENT,
	"pdf",
	{0x25, 0x50, 0x44, 0x46},
	4,
	0
  },
  {
	NICKO_TYPE_POSTSCRIPT,
	NICKO_GROUP_DOCUMENT,
	"postscript",
	{0x25, 0x21, 0x50, 0x53},
	4,
	0
  },
  {
	NICKO_TYPE_MP3,
	NICKO_GROUP_AUDIO,
	"mp3",
	{0xFF, 0xFB},
	2,
	0
  },
  {
	NICKO_TYPE_MP3,
	NICKO_GROUP_AUDIO,
	"mp3",
	{0x49, 0x44, 0x33},
	3,
	0
  },
  {
	NICKO_TYPE_FLAC,
	NICKO_GROUP_AUDIO,
	"flac",
	{0x66, 0x4C, 0x61, 0x43},
	4,
	0
  },
  {
	NICKO_TYPE_OGG,
	NICKO_GROUP_PICTURE,
	"ogg",
	{ 0x4F, 0x67, 0x67, 0x53},
	4,
	0
  },
  {
	NICKO_TYPE_MATROSKA,
	NICKO_GROUP_VIDEO,
	"matroska",
	{0x1A, 0x45, 0xDF, 0xA3},
	4,
	0
  },
  {
	NICKO_TYPE_WAV,
	NICKO_GROUP_AUDIO,
	"wav",
	{0x52, 0x49, 0x46, 0x46, '?',    '?',  '?',  '?',
	0x57, 0x41, 0x56, 0x45},
	12,
	0
  },
  {
	NICKO_TYPE_AVI,
	NICKO_GROUP_VIDEO,
	"avi",
	{0x52, 0x49, 0x46, 0x46, '?',    '?',  '?',  '?',
	 0x41, 0x56, 0x49 , 0x20},
	12,
	0
  },
  {
	NICKO_TYPE_PNG,
	NICKO_GROUP_PICTURE,
	"png",
	{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A},
	8,
	0
  },
  {
	NICKO_TYPE_JPG,
	NICKO_GROUP_PICTURE,
	"jpg",
	{0xFF, 0xD8, 0xFF, 0xE0, '?', '?', 0x4A, 0x46,
	 0x49, 0x46, 0x00, 0x01},
	12,
	0
  },
  {
	NICKO_TYPE_JPG,
	NICKO_GROUP_PICTURE,
	"jpg",
	{0xFF, 0xD8, 0xFF, '?', '?', '?', 0x45, 0x78,
	 0x69, 0x66, 0x00, 0x00},
	12,
	0
  },
  {
	NICKO_TYPE_GIF,
	NICKO_GROUP_PICTURE,
	"gif",
	{0x47, 0x49, 0x46, 0x38, 0x37, 0x61},
	6,
	0
  },
  {
	NICKO_TYPE_GIF,
	NICKO_GROUP_PICTURE,
	"gif",
	{0x47, 0x49, 0x46, 0x38, 0x39, 0x61},
	6,
	0
  },
  {
	NICKO_TYPE_ICO,
	NICKO_GROUP_PICTURE,
	"ico",
	{0x00, 0x00, 0x01, 0x00},
	4,
	0
  },
  {
	NICKO_TYPE_SCRIPT,
	NICKO_GROUP_UNSPECIFIED,
	"script",
	{'#', '!'},
	2,
	0
  },
  {
	NICKO_TYPE_MBR,
	NICKO_GROUP_UNSPECIFIED,
	"mbr",
	{0x55, 0xaa},
	2,
	510
  },
  {0, 0, NULL,  {0}, 0,  0}
};

/**
static int pread(int fd, void *buf, size_t nbyte, off_t offset)
{
  if (lseek(fd, offset, SEEK_SET) < 0)
	return -1;

  return read(fd, buf, nbyte);
}
**/

static int
_nicko_read(int fd,
		uint8_t *magic,
		struct nicko_magic *m,
		size_t size)
{
  if ((size_t)m->offset > size || m->size > size)
	return -2;

  (void)memset(magic, 0, NICKO_MAGIC_MAX);

  if (pread(fd, magic, m->size, m->offset) != (ssize_t)m->size)
	return -1;
  return 0;
}

static int
_nicko_equal(uint8_t *s1, uint8_t *s2, size_t n)
{
  size_t i = 0, ret = 0;

  for (i = 0;i < n;i++) {
	if (s2[i] == '?')
	  continue;
	ret |= s1[i] ^ s2[i];
  }
  return ret;
}

static int
_nicko_stat(const char *filename, size_t *size)
{
  struct stat st;

  if (stat(filename, &st) < 0)
	return -1;

  *size = st.st_size;

  switch (st.st_mode & S_IFMT) {
	case S_IFCHR:
	  return NICKO_TYPE_DEVICE_CHAR;
	case S_IFBLK:
	  return NICKO_TYPE_DEVICE_BLOCK;
	case S_IFDIR:
	  return NICKO_TYPE_DIR;
	case S_IFLNK:
	  return NICKO_TYPE_LINK;
	case S_IFSOCK:
	  return NICKO_TYPE_SOCKET;
	case S_IFIFO:
	  return NICKO_TYPE_PIPE;
  }

  if (st.st_size == 0)
	return NICKO_TYPE_EMPTY;

  return 0;
}

const char *nicko_get_group_name(int group)
{
  return nicko_groups[group];
}

int nicko(const char *filename, struct nicko_magic **p)
{
  int i = 8, fd = -1, st = 0, match = 0;
  size_t size = 0;
  uint8_t magic[NICKO_MAGIC_MAX];

  if ((st = _nicko_stat(filename, &size)) > 0) {
	*p = &list[st - 1];
	return 0;
  }

  if ((fd = open(filename, O_RDONLY|O_NONBLOCK)) < 0) {
	return -1;
  }

  /**
   * Read magic early so we can compare the first byte of the
   * magic.
   */
  if (_nicko_read(fd, magic, &list[i], size) < 0)
	goto end;

  for (;list[i].name;i++) {

	/**
	 * Instead of wasting time, compare only magics that starts with
	 * the same first byte. It's faster since we are not reading/
	 * seeking multiple times.
	 */
	if (list[i].offset == 0)
	  if (*magic != *list[i].magic)
	continue;

	if ((st = _nicko_read(fd, magic, &list[i], size)) == -1)
	  goto end;
	else if (st == -2)
	  continue;

	if (_nicko_equal(magic, list[i].magic, list[i].size) == 0) {
	  match = 1;
	  break;
	}
  }

end:
  if (close(fd) < 0)
	return -1;

  if (match == 0)
	return 1;

  *p = &list[i];

  return 0;
}

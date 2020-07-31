### Nicko

A simple command line tool and portable library to identify
popular file types.

### Installation

```bash
$ make
$ [sudo] make install
$ nicko --help && nicko --version
```

After the installation the library will be installed in your local path
(/usr/local):

1. /usr/local/lib/libnicko.a
2. /usr/local/include/nicko.h
3. /usr/local/bin/nicko
4. /usr/local/share/man1/nicko.1.gz

### API

There is only 1 function to worry about:

```c

int nicko(const char *filename, struct nicko_magic **p);

```

Example:

```c
#include <stdio.h>

#include <nicko.h>

int main(void)
{
  struct nicko_magic *m = NULL;

  if (nicko("/usr/bin/pwd", &m) != 0)
    return 0;

  puts(m->name);

  if (m->type == NICKO_TYPE_ELF)
    puts("elf");
  else if (m->group == NICKO_GROUP_ARCHIVE)
    puts("in group archive.");

  return 0;

}
```

Compile with:

```bash
$ cc -o nicko-test nicko-test.c -lnicko
```

Read nicko.h for the full API it is well documented.


### Command line

nicko(1) is also a command line tool (like `file(1)` but with less
features).

It is desgiend for scripts, for example:

```bash
#!/usr/bin/bash

if [[ $(nicko -t /usr/bin/ls) == "elf" ]];then
    echo "Found binary: /usr/bin/ls"
fi

# More examples

if [[ $(nicko -g /home/user/tux.png) == "picture" ]];then
    # Set wallpaper
    ...
fi
```

### Contributing

Contributions are welcome!

### Credits

1. [Wikipedia](https://en.wikipedia.org/wiki/List_of_file_signatures)

### License

Licensed under the [ISC](https://opensource.org/licenses/ISC) license.

    Copyright (c) 2018 bindh3x <os@bindh3x.io>

    Permission to use, copy, modify, and distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
    WARRANTIES
    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


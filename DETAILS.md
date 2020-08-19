### [LIBCIMBAR](https://github.com/sz3/libcimbar)
### DETAILS | [PERFORMANCE](PERFORMANCE.md) | [TODO](TODO.md)

## The premise

Cimbar begins with the image hash:

(example imagehash of one of the tiles -- including the threshold'd bits, culminating in the 64 bit number)

The image hash, in this case, simply represents each pixel in the image, lumped into a single 64 bit value (ordered by row). There are a number of different image hashing algorithms -- a threshold-based approach is perhaps the simplest. It being simple will help us later.

The image hash we choose helps us define our symbols. Each symbol needs an image hash [far away]((https://en.wikipedia.org/wiki/Hamming_distance)) from all other symbols -- and we need 2^(num_bits) symbols, where num_bits is the number of bits we wish to encode per symbol. Here is a set of 16 symbols, and the bits they encode:

![4 bit cimbar tiles](https://github.com/sz3/cimbar-samples/blob/v0.5/docs/tile-bits.png)

This is, in fact, the set cimbar uses. Each symbol is >=20 bits (by hamming distance) from all other symbols, and importantly, this relationship tends to hold (though not perfectly) even when symbols are blurry or otherwise imperfect.

That these symbols are reasonably far apart is perhaps the single most vital aspect of cimbar. When we decode, we will check a tile against the lot. If we find an unambiguously "best" symbol -- we also will have found the unambiguously "best" bits.

## Implementation: Encoder

In pseudocode, cimbar encoding looks something like this:
```
for bits in error_correction(file):
    for x, y in next_position():
        img.paste(cimbar_tile(bits), x, y)
```

The encoder iterates over the input data, assigning symbols (and perhaps colors) according to the bits it reads.

![4 bit cimbar encoding](https://github.com/sz3/cimbar-samples/blob/v0.5/docs/encoding.png)

The above is a 4x4(x4) cimbar grid -- encoding 64 bits of data. A real cimbar images looks like this:

![an example cimbar code](https://github.com/sz3/cimbar-samples/blob/v0.5/6bit/4color_ecc30_fountain_0.png)

... and contains 1550 tiles for data storage. For 6-bit cimbar (4 symbol bits, 2 color bits), this means 9300 bytes per image.

### Error Correction

We may have 9300 bytes per image, but we cannot use all of those bytes for our data payload. The decoder will do a good job matching tiles to its dictionary of symbols, but it will not be perfect. We need error correction.

As a reminder:
```
for bits in error_correction(file):
    for x, y in next_position():
        img.paste(cimbar_tile(bits), x, y)
```

The `error_correction` in question will:
	* read `155-ecc` bytes
	* add `ecc` bytes of error correction
	* use the `155` byte chunk (data + ecc_bytes) as the next set of inputs, to be torn apart and encoded 6-bits at a time

As an example, for `ecc=30`, we will have 30 bytes of error correction data for every 125 bytes of "real" data.

### Interleaving

Error correction is applied on adjacent bytes -- but errors on an image tend to cluster around adjacent cells. For example, imagine a pen, or a finger, obstructing part of the code.

Because of this characteristic, it's useful to interleave ECC chunks across the image. The implementation cimbar uses is to skip over N cells:

(animated graphic or something?)

### Fountain encoding:

What if our source file is larger than 7500 bytes? (9300 * `ecc=30`/155)

What if our source file is much smaller, and we'd like to make sure it can be decoded, even in the face of large errors?

The solution cimbar implements is done via fountain codes.

Fountain codes:

* introduce a small amount of overhead for bookkeeping purposes (in 6 bit cimbar, it is 6 bytes per 744 of "real data")
* allow the decoder to reconstruct a file over multiple fountain frames
* *regardless of what order* the multiple fountain frames are received
* even if frames are missing, as long as N+1 frames are received (where N is file_size/bytes_per_frame=744)

These properties may appear to be magical as you consider them more, and they do come with a few tradeoffs:

1. the fountain decoder defines how large a file can be
	* in wirehair+cimbar's case, 16.7MB.
2. may require the file contents to be stored in RAM
	* this relates to the size limit!

This constraint is less of an obstacle than it may seem -- the fountain codes are essentially being used as a "wire" format, and the encoder and decoder could agree on a protocol or scheme to respectively split up, and then reassemble, larger files. Cimbar does not yet implement this, however!

## Implementation: Decoder

The decoder is, naturally, the inverse of the encoder in most respects. However, it's job is more difficult. The decoder must:

* locate the cimbar code inside a candidate image

* extract the cimbar code, taking care when doing its 2D image transform to get the image "close enough" for a decode to run

* deal with misshapen, blurry, or simply unreadable symbols, while successfully decoding as much as it can.

### Scan and extract

The scanner logic will be familiar to anyone familiar with how QR Codes work. There are three 1:1:4:1:1 patterns at the corners of the image, and a 1:2:2:2:1 pattern on the bottom right corner. The primary scan is for the three corners -- if they are found, the scanner makes a guess as to the location of the 4th, and then does a secondary scan to pinpoint its exact location.

The extract is done with a perspective transform.

![after extract](https://github.com/sz3/cimbar-samples/blob/v0.5/6bit/4_30_f0_627_extract.jpg)

### Symbol decodes

There are a number of small problems that can conspire to create large problems when decoding a cimbar image.

* some tiles might be blurry
* some tiles might be too dark
* the image might be misaligned
* the image might suffer from lens distortion
* the image might be too small, meaning the tiles have lost too much definition

Notably, these problems can also be *localized* in an image, meaning that while the right half is easy enough to decode, the left half becomes a disaster of misalignments and bad-guesses leading to worse misalignments and worse guesses. Interleaving and error correction can solve some of these problems, but at a certain point it becomes too much to deal with -- short of increasing the ECC level or image resolution.

### The anatomy of a decode

Decoding begins from the corners.



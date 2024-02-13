### [LIBCIMBAR](https://github.com/sz3/libcimbar)
### DETAILS | [PERFORMANCE](PERFORMANCE.md) | [TODO](TODO.md)

## The premise

Cimbar is a grid of colored tiles. Conceptually, it is built on the idea of `image hashing`:

![example image hash](https://github.com/sz3/cimbar-samples/blob/v0.5/docs/imagehash.png)

The image hash cimbar uses is a simple threshold -- a 1 if the pixel is set, and a 0 if not. The 8x8 grid is encoded as a 64-bit number, left to right, top to bottom. There are many cleverer image hashing algorithms -- this threshold-based approach is perhaps the simplest. But simplicity is a virtue!

The image hash we choose helps us define our symbols. Each symbol needs an image hash [clearly distinct]((https://en.wikipedia.org/wiki/Hamming_distance)) from all other symbols -- and we need 2^`num_bits` symbols, where `num_bits` is the number of bits we wish to encode per symbol. Here is a set of 16 symbols, and the 4-bit strings they encode:

![4 bit cimbar tiles](https://github.com/sz3/cimbar-samples/blob/v0.5/docs/tile-bits.png)

This is the set cimbar uses. Each symbol is around 20 bits (by imagehash hamming distance) from all other symbols, and importantly, this relationship tends to hold (though not perfectly) even when symbols are blurry or otherwise corrupted.

That these symbols are distinguishable through the lens of the image hash is perhaps the single most vital aspect of cimbar. When we decode, we will check a tile against the lot. If we find an unambiguously best symbol -- we also will have found the unambiguously best bits.

## Implementation: Encoder

In pseudocode, cimbar encoding looks something like this:
```
for bits in error_correction(file):
    for x, y in next_position():
        img.paste(cimbar_tile(bits), x, y)
```

The encoder iterates over the input data, assigning symbols (and perhaps colors) according to the bits it reads.

![4 bit cimbar encoding](https://github.com/sz3/cimbar-samples/blob/v0.5/docs/encoding.png)

The above is a 4x4(x4) cimbar grid -- encoding 64 bits of data. A real cimbar image looks like this:

![an example cimbar code](https://github.com/sz3/cimbar-samples/blob/v0.5/6bit/4color_ecc30_fountain_0.png)

... and contains 12400 tiles for data. For 6-bit cimbar (4 symbol bits, 2 color bits), this means 9300 bytes per image.

### Error Correction

We may have 9300 bytes per image, but we cannot use all of those bytes for our data payload. The decoder will do a good job matching tiles to its dictionary of symbols, but it will not be perfect. We will need error correction.

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

As an example, for `ecc=30`, we will have 30 bytes of error correction data for every 125 bytes of real data.

### Interleaving

Error correction is applied on adjacent bytes -- but errors on an image tend to cluster around adjacent cells. For example, imagine a pen, or a finger, obstructing part of the code.

Because of this characteristic, it's useful to interleave ECC chunks across the image. The implementation cimbar uses is to skip over N cells.

[sorry, a helpful graphic will be added later]

### Fountain encoding:

What if our source file is larger than 7500 bytes? (9300 * `ecc=30`/155)

What if our source file is much smaller, and we'd like to make sure it can be decoded, even in the face of large errors?

The solution cimbar implements is to use [fountain codes](https://en.wikipedia.org/wiki/Fountain_code).

Fountain (wirehair) codes:

* introduce a small amount of overhead for bookkeeping purposes (in 6 bit cimbar, it is 6 bytes per 744 of real data)
* allow the decoder to reconstruct a file over multiple fountain frames
* *regardless of what order* the multiple fountain frames are received
* even if frames are missing, as long as N+1 frames are received (where N is `file_size`/`bytes_per_frame`)

These properties may appear to be magical as you consider them more, and they do come with a few tradeoffs:

1. the fountain decoder defines how large a file can be
	* in cimbar's case, capped at 33.55MB
2. wirehair requires the file contents to be stored in RAM
	* this relates to the size limit!

The size constraint is less of an obstacle than it may seem -- the fountain codes are essentially being used as a wire format, and the encoder and decoder could agree on a scheme to split up, and then reassemble, larger files. Cimbar does not (yet?) implement this, however!

## Implementation: Decoder

The decoder is, unsurprisingly, the inverse of the encoder in most respects. However, its job is more difficult. The decoder must:

* locate the cimbar code inside a candidate image

* extract the cimbar code, taking care when doing its 2D image transform to get the image "close enough" for a decode to run

* deal with misshapen, blurry, or simply unreadable symbols, while successfully decoding as much as it can.

### Scan and extract

The scan and extract are functionally similar to how QR codes work -- so I'm going to gloss over the details a bit. In summary:

* There are three square patterns at the corners of the image, and the scanner must find these. It then triangulates a range in which to run a secondary scan for the bottom-right corner.

* Once all 4 corners are located, the extract is done with a perspective transform.

![after extract](https://github.com/sz3/cimbar-samples/blob/v0.5/6bit/4_30_f0_627_extract.jpg)

### Symbol decodes: an overview

The decoder loop must actively work to minimize errors when decoding. In pseudocode, it might look something like this:
```
for i, bits, distance, drift in next_decode():
    results[deinterleave(i)] = bits
    position_tracker.update(i, drift, distance)

decoded_data = error_correct(results)
```

* `i`: the cell position
* `bits`: the decoded bits for this decode
* `distance`: the distance, or confidence, from the image hash when it selected a suitable symbol. Lower is better.
* `drift`: an (x,y) offset that tracks local distortion for this decode. It is capped at 7px in all directions.
* `deinterleave(i)` will return the "real" bit index of the bits, based on our interleave scheme -- the reverse of the encoder.
* `position_tracker.update()` here informs `next_decode()` which cells it should prioritize next. Imagine a priority_queue based on `distance`.
* `error_correct(results)` will reassemble the file, if error correction succeeds.

The reason for complexity is multi-fold:
* we have to reverse the interleave, to reassemble the file contents in the right order
* we *decode* the cells out of order, using the image hash's `distance` metric as a (inverted) *confidence* metric. To minimize errors, we want to decode cells we are more confident about before cells we are less confident about.
* the distance/confidence is used in combination with `drift` -- an (x,y) offset -- that tracks the local distortion for upcoming decodes. When we have higher confidence in a cell, its drift is preferred.

### Reasons for errors

There are a number of small problems that can conspire to create large problems when decoding a cimbar image.

* some tiles might be blurry
* some tiles might be too dark
* the image might be misaligned
* the image might suffer from lens distortion
* the image might be too small, meaning the tiles have lost too much definition (see: blurry)

Notably, these problems can also be *localized* in an image, meaning that while the right half might be easy enough to decode, the left half becomes a disaster of misalignments and bad-guesses leading to worse misalignments and worse guesses. Interleaving and error correction can solve some of these problems, but at a certain point it becomes too much to deal with -- short of increasing the ECC level or input image resolution.

### Example decode

[to be continued...]



### [INTRODUCTION](https://github.com/sz3/cimbar) | [ABOUT](https://github.com/sz3/cimbar/blob/master/ABOUT.md) | LIBCIMBAR
### [DETAILS](DETAILS.md) | [PERFORMANCE](PERFORMANCE.md) | [TODO](TODO.md)

## libcimbar: implementing Color Icon Matrix Barcodes
And the quest for 100 kb/s over the air gap...

## What is it?

`cimbar` is a proof-of-concept high-density 2D barcode format. Data is stored in a grid of colored tiles -- bits are encoded based on which tile is chosen, and which color is chosen to draw the tile with. Reed Solomon error correction is applied on the data, to account for the lossy nature of the video -> digital decoding. Sub-1% error rates are expected.

`libcimbar`, the optimized implementation, includes a simple protocol for file encoding based on fountain codes (`wirehair`). This allows for files of up to 16MB to be encoded in a series of cimbar codes, which can be output as a series of images, or generated on the fly as a "video" feed of animated cimbar codes. The magic of fountain codes means that once enough distinct images have been decoded successfully, the file will be reconstructed successfully. This is true even if the images are out of order, or if random images have been corrupted or are missing.

## Platforms

The code is written in C++, and developed/tested on amd64+linux and arm64+android. It probably works, or can be made to work, on other platforms. Maybe.

I would like to add emscripten->wasm support.

## Library dependencies

[OpenCV](https://opencv.org/) must be installed before building. All other dependencies are included in the source tree.

* opencv - https://opencv.org/
* base - https://github.com/r-lyeh-archived/base
* catch2 - https://github.com/catchorg/Catch2
* concurrentqueue - https://github.com/cameron314/concurrentqueue
* cxxopts - https://github.com/jarro2783/cxxopts (used for command line tools)
* fmt - https://github.com/fmtlib/fmt
* intx - https://github.com/chfast/intx
* libcorrect - https://github.com/quiet/libcorrect
* libpopcnt - https://github.com/kimwalisch/libpopcnt
* PicoSHA2 - https://github.com/okdshin/PicoSHA2 (used for testing)
* wirehair - https://github.com/catid/wirehair

## Build

```
cmake .
make -j4
```

## Usage

Encode:

```
./cimbar --encode -i inputfile.pdf -o outputprefix -f
```

Decode (extracts file into output directory):
```
./cimbar outputprefix-1.png outputprefix-2.png outputprefix-3.png -o /tmp -f
```

Encode and animate to window:
```
./cimbar_video inputfile.pdf
```

## Performance numbers

[PERFORMANCE](PERFORMANCE.md)

## Implementation details

[DETAILS](DETAILS.md)

## Room for improvement/next steps

[TODO](TODO.md)

## Inspiration

* https://github.com/JohannesBuchner/imagehash/
* https://github.com/divan/txqr
* https://en.wikipedia.org/wiki/High_Capacity_Color_Barcode

## Would you like to know more?

### [INTRODUCTION](https://github.com/sz3/cimbar) | [ABOUT](https://github.com/sz3/cimbar/ABOUT.md)

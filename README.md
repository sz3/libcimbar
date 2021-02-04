### [INTRODUCTION](https://github.com/sz3/cimbar) | [ABOUT](https://github.com/sz3/cimbar/blob/master/ABOUT.md) | [CFC](https://github.com/sz3/cfc) | LIBCIMBAR
### [DETAILS](DETAILS.md) | [PERFORMANCE](PERFORMANCE.md) | [TODO](TODO.md)

## libcimbar: Color Icon Matrix Barcodes

Behold: an experimental barcode format for air-gapped data transfer.

It can sustain speeds of 770+ kilobits/s (~96 KB/s) using nothing but a smartphone camera!

## Explain?

The encoder outputs an animated barcode to a computer or smartphone screen:
* Encoder web app: https://cimbar.org

While the decoder is a cell phone app that uses the phone camera to read the animated barcode:
* Decoder android app: https://github.com/sz3/cfc

No internet/bluetooth/NFC/etc is used. All data is transmitted through the camera lens. You can try it out yourself, or take my word that it works. :)

## How does it work?

`cimbar` is a high-density 2D barcode format. Data is stored in a grid of colored tiles -- bits are encoded based on which tile is chosen, and which color is chosen to draw the tile. Reed Solomon error correction is applied on the data, to account for the lossy nature of the video -> digital decoding. Sub-1% error rates are expected, and corrected.

`libcimbar`, this optimized implementation, includes a simple protocol for file encoding based on fountain codes (`wirehair`). Files of up to 33MB can be encoded in a series of cimbar codes, which can be output as images or a live video feed. Once enough distinct image frames have been decoded successfully, the file will be reconstructed successfully. This is true even if the images are received out of order, or if some have been corrupted or are missing.

## Platforms

The code is written in C++, and developed/tested on amd64+linux, arm64+android, and emscripten+wasm. It probably works, or can be made to work, on other platforms.

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
* stb_image - https://github.com/nothings/stb (for loading embedded pngs)
* wirehair - https://github.com/catid/wirehair
* zstd - https://github.com/facebook/zstd

Optional:
* GLFW - https://github.com/glfw/glfw or `libglfw3-dev` (for when `opencv-highgui` is not available or unwanted)
    * the GLFW code path also needs/uses `GLES3/gl3.h` (`libgles2-mesa-dev` on ubuntu 18.04)

## Build

```
cmake .
make -j7
make install
```

By default, libcimbar will try to install build products under `./dist/bin/`.

To build the emscripten+WASM encoder (what cimbar.org uses), see [WASM](WASM.md).

## Usage

Encode:
* large input files may fill up your disk with pngs!

```
./cimbar --encode -i inputfile.txt -o outputprefix -f
```

Decode (extracts file into output directory):
```
./cimbar outputprefix*.png -o /tmp -f
```

Encode and animate to window:
```
./cimbar_send inputfile.pdf
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

### [INTRODUCTION](https://github.com/sz3/cimbar) | [ABOUT](https://github.com/sz3/cimbar/blob/master/ABOUT.md)

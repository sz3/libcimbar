### [INTRODUCTION](https://github.com/sz3/cimbar) | [ABOUT](https://github.com/sz3/cimbar/blob/master/ABOUT.md) | [CFC](https://github.com/sz3/cfc) | LIBCIMBAR
### [DETAILS](DETAILS.md) | [PERFORMANCE](PERFORMANCE.md) | [TODO](TODO.md)

## libcimbar: Color Icon Matrix Barcodes

Behold: an experimental barcode format for air-gapped data transfer.

It can sustain speeds of 850 kilobits/s (~106 KB/s) using just a computer monitor and a smartphone camera!

<p align="center">
<img src="https://github.com/sz3/cimbar-samples/blob/v0.6/b/4cecc30f.png" width="70%" title="A non-animated mode-B cimbar code" >
</p>

## Explain?

The encoder outputs an animated barcode to a computer or smartphone screen:
* Encoder web app: https://cimbar.org

While the decoder is a cell phone app that uses the phone camera to read the animated barcode:
* Decoder android app: https://github.com/sz3/cfc

No internet/bluetooth/NFC/etc is used. All data is transmitted through the camera lens. You can try it out yourself, or take my word that it works. :)

## How does it work?

`cimbar` is a high-density 2D barcode format. Data is stored in a grid of colored tiles -- bits are encoded based on which tile is chosen, and which color is chosen to draw the tile. Reed Solomon error correction is applied on the data, to account for the lossy nature of the video -> digital decoding. Sub-1% error rates are expected, and corrected.

`libcimbar`, this optimized implementation, includes a simple protocol for file encoding built on fountain codes (`wirehair`) and zstd compression. Files of up to 33MB (after compression!) are encoded in a series of cimbar codes, which can be output as images or a live video feed. Once enough distinct image frames have been decoded successfully, the file will be reconstructed and decompressed successfully. This is true even if the images are received out of order, or if some have been corrupted or are missing.

## Platforms

The code is written in C++, and developed/tested on amd64+linux, arm64+android (decoder only), and emscripten+WASM (encoder only). It probably works, or can be made to work, on other platforms.

Crucially, because the encoder compiles to asmjs and wasm, it can run on anything with a modern web browser. For offline use, you can either install cimbar.org as a progressive web app, or [download the latest release](https://github.com/sz3/libcimbar/releases/latest) of `cimbar_js.html`, save it locally, and open it in your web browser.

## Library dependencies

[OpenCV](https://opencv.org/) and [GLFW](https://github.com/glfw/glfw) (+ OpenGL ES headers) must be installed before building. All other dependencies are included in the source tree.

* opencv - https://opencv.org/ (`libopencv-dev`)
* GLFW - https://github.com/glfw/glfw (`libglfw3-dev`)
* GLES3/gl3.h - `libgles2-mesa-dev`
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

## Build

1. install opencv and GLFW. On ubuntu/debian, this looks like:
```
sudo apt install libopencv-dev libglfw3-dev libgles2-mesa-dev
```

2. run the cmake + make incantation
```
cmake .
make -j7
make install
```

By default, libcimbar will try to install build products under `./dist/bin/`.

To build cimbar.js (what cimbar.org uses), see [WASM](WASM.md).

## Usage

Encode:
* large input files may fill up your disk with pngs!

```
./cimbar --encode -i inputfile.txt -o outputprefix
```

Decode (extracts file into output directory):
```
./cimbar outputprefix*.png -o /tmp
```

Decode a series of encoded images from stdin:
```
echo outputprefix*.png | ./cimbar -o /tmp
```

Encode and animate to window:
```
./cimbar_send inputfile.pdf
```

You can also encode a file using [cimbar.org](https://cimbar.org), or the latest [release](https://github.com/sz3/libcimbar/releases/latest).

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

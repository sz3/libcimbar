### [INTRODUCTION](https://github.com/sz3/cimbar) | [ABOUT](ABOUT.md) | LIBCIMBAR

## libcimbar: implementing Color Icon Matrix Barcodes
#### And the quest for 100 kb/s over the air gap

## What is it?

`cimbar` is a proof-of-concept high-density 2D barcode format. Data is stored in a grid of colored tiles -- bits are encoded based on which tile is chosen, and which color is chosen to draw the tile with. Reed Solomon error correction is applied on the data, to account for the lossy nature of the video -> digital decoding. Sub-1% error rates are expected.

`libcimbar` also includes a simple protocol for file encoding based on fountain codes (`wirehair`). This allows for files of up to 16MB to be encoded in a series of cimbar codes, which can be output as a series of images, or generated on the fly as a "video" feed of animated cimbar codes. The magic of fountain codes means that once enough distinct images have been decoded successfully, the file will be reconstructed successfully. This is true even if the images are out of order, or if random images have been corrupted or are missing.

## Platforms

The code is developed/tested on amd64+linux and arm64+android. It probably works, or can be made to work, on other platforms. But YMMV.

I would like to add emscripten->wasm support.

## Library dependencies

[OpenCV](https://opencv.org/) must be installed before building. The rest are included in the source tree.

* opencv - https://opencv.org/
* base - https://github.com/r-lyeh-archived/base
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

```
```

## Numbers of note

* The barcode is `1024x1024` pixels. The individual tiles are `8x8` in a `9x9` grid (there is an empty row/column of spacing on either side)
* **7500** or 8750 bytes per cimbar image, after error correction
	* There are 16 possible symbols per tile, encoding 4 bits
	* There are 4 or 8 possible colors, encoding an additional 2-3 bits per tile.
	* These 6-7 bits per tile work out to a maximum of 9300-10850 bytes per barcode, though in practice this number is reduced by error correction.
	* The default ecc setting is 30/155, which is where we go from 9300 -> 7500 bytes of real data for a 4-color cimbar image.
		* Reed Solomon is not an ideal for this use case -- specifically, it corrects byte errors, and cimbar errors tend to involve 1-3 bits at a time. However, since Reed Solomon implementations are ubiquitous, I used it for this prototype.
* 43 seconds to transfer a 3.0 MB file == 69 kb/s. This is the current benchmark on my old Android phone.
	* specifically, this is using https://github.com/sz3/cfc, running with 4 CPU threads on a Qualcomm Snapdragon 625
	* for smaller (~1MB) files, I've seen 75 kb/s.
		* for inflating benchmark numbers, a decent strategy is to turn down the ecc settings and say a prayer to your prefered deity
			* this is neither reliable, nor representative of the usable case, however
	* the android app is not currently using the GPU. Also, it's a pretty minimal effort app build on an opencv tutorial. It kind of sucks.
		* notably: the app doesn't do anything useful with camera settings. It would help processing if the app was handing libcimbar better quality images!

## Implementation details

(to be written. Probably a separate md file?)

## Room for improvement/next steps

libcimbar is fairly optimized, to achieve the "proof" part of proof-of-concept. Specifically, it felt unreasonable to claim that it could be used to transfer MBs of data over a camera lens (on a non-geologic time scale) without seeing performance numbers to back that up.

Performance optimizations aside, there are a number of paths that might be interesting to pursue. Some I may take a look at, but most I will leave to any enterprising developer who wants to take up the cause:

* emscripten+wasm
	* specifically, would like to be able to encode cimbar codes in the browser
* proper metadata/header information?
	* would be nice to be able to determine ecc/#colors/#smybols from the cimbar image itself?
	* The bottom right corner is the obvious place to reclaim space to make this possible.
* multi-frame decoding?
	* when decoding a static cimbar image, it would be useful to be able to use prior (unsuccessful) decode attempts to inform a future decode, and -- hopefully -- increase the probability of success. Currently, all frames are decoded independently.
		* there is already a granular confidence metric that could be reused -- the `distance` that's tracked when decoding symbol tiles...
* optimal symbol set?
	* the 16-symbol (4 bit) set is hand-drawn. I stared with ~40 or so hand-drawn symbols, and used the 16 that performed best with each other.
	* there is surely a more optimal set -- a more rigorous approach should yield lower error rates!
	* but, more importantly, it may be possible to go up to 32 symbols, and encode 5 bits per tile?
* optimal symbol size?
	* the symbols that make up each cell on the cimbar grid are 8x8 (in a 9x9 grid).
	* this is because imagehash was on 8x8 tiles!
	* smaller sizes might also work?
	* the limiting factor is the hamming distance between each image hash "bucket", and the 9Xth percentile decoding errors.
* optimal color set?
	* the 4-color (2 bit) pallettes seem reasonable. 8-color, perhaps less so?
	* this may be a limitation of the algorithm/approach, however. Notably, since each symbol is drawn with one pallette color, all colors need sufficient contrast against the backdrop (#000 or #FFF, depending). This constrains the color space somewhat, and less distinct colors == more errors.
* optimal grid size?
	* 1024x1024 is a remnant of the early prototyping process. There is nothing inherently special about it (except that it fits on a 1920x1080 screen, which seems good)
		* the tile grid itself is 1008x1008 (1008 == 9x112 -- there are 112 tile rows and columns)
	* a smaller grid would be less information dense, but more resilient to errors. Probably.
* optimal grid shape?
	* it's a square because QR codes are square. That's it. Should it be?
* more efficient ECC?
	* LDPC?
	* Reed Solomon operates on bytes. Most decode errors tend to average out at 1-3 bits. It's not a total disaster, because it works. However, it would be nice to have denser error correction codes.
* crack the seal on OpenCV+OpenCL2.0+SVM, to enable proper GPU support on android?
	* it would be nice to use the GPU on android. It currently is not reasonable because of the time cost of memory buffer copies between the GPU->CPU.
* ???
	* still reading? Of course there's more! There's always more!

## Inspiration

* https://github.com/JohannesBuchner/imagehash/
* https://github.com/divan/txqr
* https://en.wikipedia.org/wiki/High_Capacity_Color_Barcode (if MS had already open sourced this, I wouldn't have bothered to invent my own)


## libcimbar
#### Color Icon Matrix BARcode
#### And the quest for 100 kb/s over the air gap

## Libraries
Except for opencv, all of these are included in the repo. Most are header-only.

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

## What is it?

`cimbar` is a proof-of-concept high-density 2D barcode format. Data is stored in a grid of colored tiles -- bits are encoded based on which of tile is chosen, and which color is chosen to draw the tile with. Reed Solomon error correction is applied on the data, to account for the lossy nature of the video -> digital decoding. Nonetheless, sub-1% error rates are expected.

There are multiple color schemes:
	* "dark" mode -- meant for backlit computer screens, with bright tiles on a black background
	* "light" mode -- meant for paper, with dark tiles on a white background

`libcimbar` also includes a simple protocol for file encoding based on fountain codes (`wirehair`). This allows for files up to 16MB to be encoded in a series of cimbar codes, which can be output as a series of images, or generated on the fly as a "video" feed of animated cimbar codes. The magic of fountain codes means that once enough distinct images have been decoded successfully, the file will be reconstructed successfully. This is true even if the images are out of order, or if random images have been corrupted or are missing.

## Platforms

The code is developed/tested on amd64+linux and arm64+android. It probably works, or can be made to work, on other platforms. But YMMV.

I would like to add emscripten->wasm support.

## Numbers of note

* The barcode is `1024x1024` pixels. The individual tiles are `8x8` in a `9x9` grid (there is an empty row/column of spacing on either side)
* 7500-8750 bytes per cimbar image, after error correction
	* There are 16 possible symbols per tile, encoding 4 bits
	* There are 4 or 8 possible colors, encoding an additional 2-3 bits per tile.
	* These 6-7 bits per tile work out to a maximum of 9300-10850 bytes per barcode, though in practice this number is reduced by error correction.
	* The default ecc setting is 30/155, which is where we go from 9300 -> 7500 bytes of real data for a 4-color cimbar image.
		* Reed Solomon is not an ideal for this use case -- specifically, it corrects byte errors, and cimbar errors tend to involve 1-3 bits at a time. However, since Reed Solomon implementations are ubiquitous, I used it for this prototype.
* 18 seconds to transfer a 1.2 MB file == 66 kb/s. This is the current benchmark on my old Android phone.
	* specifically, this is using https://github.com/sz3/cfc, running with 4 CPU threads on a Qualcomm Snapdragon 625
	* this is not using the GPU. Also, the android app kind of sucks.

## Implementation details

(to be written. Probably a separate md file?)

## Room for improvement/next steps

libcimbar is fairly optimized, to achieve the "proof" part of proof-of-concept. Specifically, it felt unreasonable to claim that it could be used to transfer MB of data over a camera lens without seeing performance numbers to back that up.

Performance optimizations aside, there are a number of paths that might be interesting to pursue:

* emscripten+wasm
	* specifically, would like to be able to encode cimbar codes in the browser
* proper metadata/header information?
	* would be nice to be able to determine ecc/#colors/#smybols from the cimbar image itself?
	* The bottom right corner is the obvious place to reclaim space to make this possible.
* optimal symbol set?
	* the 16-symbol (4 bit) set is hand-drawn, based on some experiments baseed on ~40 or so symbols to start with.
	* there is surely a more optimal set.
	* but, more importantly, it may be possible to go up to 32 symbols, and encode 5 bits per tile?
* optimal symbol size?
	* the symbols that make up each cell on the cimbar grid are 8x8 (in a 9x9 grid).
	* this is because imagehash was on 8x8 tiles!
	* smaller sizes might work better?
	* the limiting factor is the hamming distance between each "target" image hash, and the average errors that 
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
	* Reed Solomon operates on bytes. Most decode errors tend to average out at 2-3 bits. It's not a total disaster, because it works. However, it would be nice to have denser error correction codes.
* crack the seal on OpenCV+OpenCL2.0+SVM, to enable proper GPU support on android?
	* it would be nice to use the GPU on android. It currently is not reasonable because of the time cost of memory buffer copies between the GPU->CPU.
* ???
	* still reading? Of course there's more! There's always more!

## Inspiration

https://github.com/JohannesBuchner/imagehash/
https://github.com/divan/txqr

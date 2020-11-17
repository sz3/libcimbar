### [LIBCIMBAR](https://github.com/sz3/libcimbar)
### [DETAILS](DETAILS.md) | PERFORMANCE | [TODO](TODO.md)

## Numbers of note

* The barcode is `1024x1024` pixels. The individual tiles are `8x8` in a `9x9` grid (there is an empty row/column of spacing on either side)
* **7500** or 8750 bytes per cimbar image, after error correction
	* There are 16 possible symbols per tile, encoding 4 bits
	* There are 4 or 8 possible colors, encoding an additional 2-3 bits per tile.
	* These 6-7 bits per tile work out to a maximum of 9300-10850 bytes per barcode, though in practice this number is reduced by error correction.
	* The default ecc setting is 30/155, which is how we go from 9300 -> 7500 bytes of real data for a 4-color cimbar image.
		* Reed Solomon is not an ideal for this use case -- specifically, it corrects byte errors, and cimbar errors tend to involve 1-3 bits at a time. However, since Reed Solomon implementations are ubiquitous, I used it for this prototype.

## Current sustained benchmark

* 4-color cimbar with ecc=30:
	* 2,980,556 bytes (after compression) in 36s -> 662 kilobits/s

* 8-color cimbar with ecc=30:
	* 2,980,556 bytes in 31s -> 769 kilobits/s

* details:
	* these numbers are use https://github.com/sz3/cfc, running with 4 CPU threads on a Qualcomm Snapdragon 625
		* perhaps I will buy a new cell phone to inflate the benchmark numbers.
	* the sender commandline is `./cimbar_send /path/to/file -s`
		* the `shakycam` option allows cfc to quickly discard ghosted frames, and spend more time decoding real data.
	* burst rate can be higher (or lower)
		* to this end, lower ecc settings *can* provide better burst rates
	* 8-color cimbar is considerably more sensitive to lighting conditions. Notably, decoding has some issues with dim screens.


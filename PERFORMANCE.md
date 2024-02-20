### [LIBCIMBAR](https://github.com/sz3/libcimbar)
### [DETAILS](DETAILS.md) | PERFORMANCE | [TODO](TODO.md)

## Numbers of note

* The barcode is `1024x1024` pixels. The individual tiles are `8x8` in a `9x9` grid (there is an empty row/column of spacing on either side)
* **7500** bytes per cimbar image, after error correction
	* There are 16 possible symbols per tile, encoding 4 bits
	* There are 4 or 8 possible colors, encoding an additional 2-3 bits per tile.
	* These 6 bits per tile work out to a maximum of 9300 bytes per barcode, though in practice this number is reduced by error correction.
	* The default ecc setting is 30/155, which is how we go from 9300 -> 7500 bytes of real data for a 4-color cimbar image.
		* Reed Solomon is not perfect for this use case -- specifically, it corrects byte errors, and cimbar errors tend to involve 1-3 bits at a time. However, since Reed Solomon implementations are ubiquitous, it is currently in use.

## Current sustained benchmark

* `mode B` (8x8 4-color) cimbar with ecc=30/155:
	* 4,689,084 bytes (after compression) in 44s -> 852 kilobits/s (~106 KB/s)
	* mode B was introduced in 0.6.0, and should work in a wide variety of scenarios

* *legacy* `mode 4C` (8x8 4-color) cimbar with ecc=30/155:
	* 4,717,525 bytes (after compression) in 45s -> 838 kilobits/s (~104 KB/s)
	* the original configuration. Mostly replaced by mode B.

* *deprecated* `mode 8C` (8x8 8-color) cimbar with ecc=30/155:
	* 4,717,525 bytes in 40s -> 943 kilobits/s (~118 KB/s)
	* removed in 0.6.0. 8-color has always been inconsistent, and needs future research

* *beta* `mode S` (5x5 4-color) cimbar with ecc=40/216 (note: not finalized, and requires a special build)
	* safely >1 Mbit/s
	* format still a WIP. To be continued...

* details:
	* cimbar has built-in compression using zstd. What's being measured here is bits over the wire, e.g. data after compression is applied.
	* these numbers are using https://github.com/sz3/cfc, running with 4 CPU threads on a venerable Qualcomm Snapdragon 625
		* more modern cell CPUs run the decoder more quickly, but it turns out that this does not benefit performance much: the camera is usually the bottleneck.
	* the sender is the cimbar.org wasm implementation. An equivalent command line is `./cimbar_send /path/to/file`
		* cimbar.org uses the `shakycam` option to allow the receiver to detect/discard "in between" frames as part of the scan step. This allows it to spend more processing time decoding real data.
	* burst rate can be higher (or lower)
		* to this end, lower ecc settings *can* provide better burst rates. I've aimed for a balance of performance and reliability.
	* cimbar `mode B` is preferred, and should be the most reliable.
	* The older `mode 4C` *may* give more consistent transfer speeds in certain scenarios, but is mostly included for backwards-compatibility reasons.

* other notes:
	* having better lighting in the frame often leads to better results -- this is why cimbar.org has a (mostly) white background. cfc uses android's auto-exposure, auto-focus, etc (it's a demo app). Good ambient light -- or a white background -- can lead to more consitent quality frame capture.
		* screen brightness on the sender is good, but ambient light is better.
	* because of the lighting/exposure question, landscape *may* be better than portrait.
	* the cimbar frame should take up as much of the display as possible (trust the guide brackets)
		* the format is designed to decode at resolutions as low as 700x700, but performance may suffer.
	* similarly, it's best to keep the camera angle straight-on -- instead of at an angle -- to decode the whole image successfully. Decodes should still happen at higher angles, but the "smaller" part of the image may have more errors than the ECC can deal with.
	* other things to be wary of:
		* glare from light sources.
		* shaky hands.

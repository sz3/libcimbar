### [LIBCIMBAR](https://github.com/sz3/libcimbar)
### [DETAILS](DETAILS.md) | PERFORMANCE | [TODO](TODO.md)

## Numbers of note

* The barcode is `1024x1024` pixels. The individual tiles are `8x8` in a `9x9` grid (there is an empty row/column of spacing on either side)
* **7500** or 8750 bytes per cimbar image, after error correction
	* There are 16 possible symbols per tile, encoding 4 bits
	* There are 4 or 8 possible colors, encoding an additional 2-3 bits per tile.
	* These 6-7 bits per tile work out to a maximum of 9300-10850 bytes per barcode, though in practice this number is reduced by error correction.
	* The default ecc setting is 30/155, which is where we go from 9300 -> 7500 bytes of real data for a 4-color cimbar image.
		* Reed Solomon is not an ideal for this use case -- specifically, it corrects byte errors, and cimbar errors tend to involve 1-3 bits at a time. However, since Reed Solomon implementations are ubiquitous, I used it for this prototype.

## Are we 100 kb/s yet?

* No. :|

* 43 seconds to transfer a 3.0 MB file == 69 kb/s. This is the current benchmark on my old Android phone.
	* specifically, this is using https://github.com/sz3/cfc, running with 4 CPU threads on a Qualcomm Snapdragon 625
	* for smaller (~1MB) files, I've seen 75 kb/s.
		* for inflating benchmark numbers, a decent strategy is to turn down the ecc settings and say a prayer to your prefered deity
			* this is neither reliable, nor representative of the usable case, however

* we have good excuses:
	* the android app is not currently using the GPU. OpenCV nominally supports OpenCL, but its SVM (shared virtual memory) support appears to be incomplete. In any case, the time cost of moving data between CPU memory and GPU memory makes the OpenCV+GPU non-viable on android (for now). As far as I know, this is a software problem, not a hard technical limitation.
	* my test android app (cfc) is barbones, and kind of sucks: it doesn't do anything useful with camera settings. We capture at ~25 fps, and are forced to discard a full 60% of the images. A 100% success rate won't happen -- the camera capture rate and the screen display rate are not going to be perfectly aligned, resulting in some number of screen-torn frames. But I'm pretty confident we can do better than 40%!
		* a bit of editorial warning may be in order: the android camera APIs are, to put it charitably, a raging tire fire

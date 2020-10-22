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

## Are we 100 kb/s yet?

* No. :|
	* well, if you include compression, yes! Nailed it! :sunglasses:

* 43 seconds to transfer a 3.0 MB file == 69 kb/s. This is the current benchmark on my old Android phone.
	* specifically, this is using https://github.com/sz3/cfc, running with 4 CPU threads on a Qualcomm Snapdragon 625
		* perhaps I will buy a new cell phone to inflate the benchmark numbers.
	* for smaller (~1MB) files, I've seen 75 kb/s.
		* for artificial benchmark numbers, a decent strategy is to turn down the ecc settings and say a prayer to your prefered deity
			* this is neither reliable, nor representative of the usable case, however.

* we have semi-reasonable excuses:
	* the android app (cfc) is not currently using the GPU. OpenCV supports OpenCL (though you need a custom build on Android), but it's unclear to me how to copy data from the GPU -> CPU without bottlenecking system resources (ex: a 90ms pause every frame).
	* additionally, cfc doesn't do anything useful with camera settings. We capture at ~25 fps, and are forced to discard a full 60% of the images due various ghosting and auto-focus issues. A 100% success rate won't happen -- some auto-focus will always be useful, and camera capture rate and the screen display rate are not going to be perfectly aligned, resulting in some number of screen-torn frames. But I'm pretty confident we can do better than 40%!
		* editorial comment: the android camera APIs are, to put it charitably, a raging tire fire
		* cfc is currently using the legacy "camera" API. The camera2 API has eluded my comprehension.

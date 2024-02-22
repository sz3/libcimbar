### [LIBCIMBAR](https://github.com/sz3/libcimbar)
### [DETAILS](DETAILS.md) | [PERFORMANCE](PERFORMANCE.md) | TODO

## Room for improvement/next steps

libcimbar is fairly optimized, to achieve the *proof* part of proof-of-concept. Specifically, it felt unreasonable to claim that it could be used to transfer MBs of data over a camera lens (on a non-geologic time scale) without seeing performance numbers to back that up.

Performance optimizations aside, there are a number of paths that might be interesting to pursue. Some I may take a look at, but most I will leave to any enterprising developer who wants to take up the cause:

* proper metadata/header information?
	* would be nice to be able to determine ecc/#colors/#symbols from the cimbar image itself?
	* The bottom right corner is the obvious place to reclaim space to make this possible.
	* this is complicated by potential aspect ratio changes for future cimbar modes.
* multi-frame decoding?
	* when decoding a static cimbar image, it would be useful to be able to use prior (unsuccessful) decode attempts to inform a future decode, and -- hopefully -- increase the probability of success. Currently, all frames are decoded independently.
		* there is already a granular confidence metric that could be reused -- the `distance` that's tracked when decoding symbol tiles...
* optimal symbol set?
	* the 16-symbol (4 bit) set is hand-drawn. I stared with ~40 or so hand-drawn symbols, and used the 16 that performed best with each other.
	* there is surely a more optimal set -- a more rigorous approach should yield lower error rates!
	* but, more importantly, it may be possible to go up to 32 symbols, and encode 5 symbol bits per tile?
* optimal symbol size?
	* the symbols that make up each cell on the cimbar grid are 8x8 (in a 9x9 grid). this is because imagehash was on 8x8 tiles!
	* smaller sizes might also work? I've been looking into 5x5 (in a 6x6 grid) as a starting point. It seems promising.
	* the limiting factor is the hamming distance between each image hash "bucket", and the 9Xth percentile decoding errors.
* optimal color set?
	* the 4-color (2 bit) pallettes seem reasonable. 8-color, perhaps less so?
	* this may be a limitation of the algorithm/approach, however. Notably, since each symbol is drawn with one pallette color, all colors need sufficient contrast against the backdrop (#000 or #FFF, depending). This constrains the color space somewhat, and less distinct colors == more errors.
	* in addition to contrast, there is interplay between the overall brightness of the image and the exposure time needed for high framerate capture. More clean frames == more troughput.
	* the camera framerate in the CFC app is limited by auto-exposure and auto-focus behavior. A newer/better decoder app might be helpful.
* optimal grid size?
	* 1024x1024 is a remnant of the early prototyping process. There is nothing inherently special about it (except that it fits on a 1920x1080 screen, which seems good)
		* the tile grid itself is 1008x1008 (1008 == 9x112 -- there are 112 tile rows and columns)
	* a smaller grid *could* be more resilient to errors, at the expense of data capacity.
* optimal grid shape?
	* it's a square because QR codes are square. That's it. Should it be?
	* I'm strongly considering 4:3 for the next revision.
* more efficient ECC?
	* QC-LDPC?
	* Reed Solomon operates on bytes. Most decode errors tend to average out at 1-3 bits. (In the pathological case, a single read error will span two bytes.) It's not a total disaster -- it still works. 
	* I expect that state of the art ECC will allow 6-15% better throughput.
		* it's a wide range due to various unknowns (unknowns to me, anyway)
* proper GPU support (OpenCV + openCL) on android?
	* It *might* be useful. [CFC]((https://github.com/sz3/cfc) is the current test bed for this.
* wasm decoder?
	* android is going to kick CFC out of the store! (testing requirement)
		* so it might be time to write this...
	* probably needs to use Web Workers
	* in-browser GPGPU support would be interesting (but I'm not counting on it)
* ???
	* still reading? Of course there's more! There's always more!



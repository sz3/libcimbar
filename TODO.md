### [LIBCIMBAR](https://github.com/sz3/libcimbar)
### [DETAILS](DETAILS.md) | [PERFORMANCE](PERFORMANCE.md) | TODO

## Room for improvement/next steps

libcimbar is fairly optimized, to achieve the *proof* part of proof-of-concept. Specifically, it felt unreasonable to claim that it could be used to transfer MBs of data over a camera lens (on a non-geologic time scale) without seeing performance numbers to back that up.

Performance optimizations aside, there are a number of paths that might be interesting to pursue. Some I may take a look at, but most I will leave to any enterprising developer who wants to take up the cause:

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
	* I'm strongly considering 4:3 for the next revision
* more efficient ECC?
	* QC-LDPC?
	* Reed Solomon operates on bytes. Most decode errors tend to average out at 1-3 bits. In the pathological case, such a bit error will span two bytes. It's not a total disaster, because it works. However, it would be nice to use state of the art ECC.
* proper GPU support (OpenCV + openCL) on android?
* ???
	* still reading? Of course there's more! There's always more!



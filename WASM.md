## Demo encoder

[cimbar.org](https://cimbar.org)

## Releases

Both wasm and asm.js releases are available [here](https://github.com/sz3/libcimbar/releases/tag/latest). The wasm build is what cimbar.org uses. The asm.js build can be downloaded, extracted, and run locally.

## Build

To build opencv.js (and the static libraries we'll need to build against opencv)...
```
cd /path/to/opencv
mkdir opencv-build-wasm
cd opencv-build-wasm
python ../platforms/js/build_js.py build_wasm --build_wasm --emscripten_dir=/path/to/emscripten
```

With opencv.js built:
```
mkdir build-wasm
cd build-wasm
source /path/to/emscripten/emsdk/emsdk_env.sh
emcmake cmake .. -DUSE_WASM=1 -DOPENCV_DIR=/path/to/opencv
emcmake make -j7 install
```

(do `-DUSE_WASM=2` to use asm.js instead of wasm)

## What about a WASM cimbar decoder?

Some day!

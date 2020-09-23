```
mkdir build-wasm
cd build-wasm
source /path/to/emscripten/emsdk/emsdk_env.sh
emcmake cmake .. -DUSE_WASM=1 -DOPENCV_DIR=/path/to/opencv
emcmake make -j7 install
```


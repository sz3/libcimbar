#!/bin/bash
#docker run --mount type=bind,source="$(pwd)",target="/usr/src/app" -it emscripten/emsdk:3.1.39

cd /usr/src/app

apt update
apt install python3 -y

cd opencv4/
mkdir opencv-build-wasm
cd opencv-build-wasm
python3 ../platforms/js/build_js.py build_wasm --build_wasm --emscripten_dir=/emsdk/upstream/emscripten

cd /usr/src/app
mkdir build-wasm
cd build-wasm
emcmake cmake .. -DUSE_WASM=1 -DOPENCV_DIR=/usr/src/app/opencv4
make -j5 install
(cd ../web/ && tar -czvf cimbar.wasm.tar.gz cimbar_js.* index.html main.js)

cd /usr/src/app
mkdir build-asmjs
cd build-asmjs
emcmake cmake .. -DUSE_WASM=2 -DOPENCV_DIR=/usr/src/app/opencv4
make -j5 install
(cd ../web/ && zip cimbar.asmjs.zip cimbar_js.js index.html main.js)

(cd ../ && python3 package-cimbar-html.py)


#!/bin/sh
# docker run --mount type=bind,source="$(pwd)",target="/usr/src/app" -it alpine

cd /usr/src/app
apk update
apk add ccache git linux-headers vim ctags gdb py-pip musl musl-dev build-base unzip wget cmake make ninja linux-headers pkgconf

git clone https://github.com/opencv/opencv.git --depth 1 -b 4.5.5 opencv4
(cd opencv4 && mkdir build-musl && cd build-musl && cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_SHARED_LIBS=OFF -DOPENCV_GENERATE_PKGCONFIG=YES -DOPENCV_FORCE_3RDPARTY_BUILD=YES && make -j5 install)

mkdir build-musl && cd build-musl
cmake .. -DOPENCV_DIR=/usr/local/include/opencv4 -DBUILD_STATIC_LINUX=1

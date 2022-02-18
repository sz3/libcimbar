#!/bin/sh
# docker run --mount type=bind,source="$(pwd)",target="/usr/src/app" -it ubuntu:18.04

cd /usr/src/app
apt update
apt install -y pkgconf g++ cmake
apt install -y libgles2-mesa-dev libglfw3-dev

cd opencv4/
mkdir build-portable/ && cd build-portable/
cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_SHARED_LIBS=OFF -DOPENCV_GENERATE_PKGCONFIG=YES -DOPENCV_FORCE_3RDPARTY_BUILD=YES
make -j5 install

cd /usr/src/app
mkdir build-portable/ && cd build-portable/
cmake .. -DBUILD_PORTABLE_LINUX=1
make -j5 install

#!/bin/bash
## targeting old glibc
# docker run --mount type=bind,source="$(pwd)",target="/usr/src/app" -it ubuntu:16.04

SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")
cd $SCRIPT_DIR

# https://gist.github.com/jlblancoc/99521194aba975286c80f93e47966dc5
apt update
apt install -y software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test

apt update
apt install -y pkgconf g++-7 python-pip
apt install -y libgles2-mesa-dev libglfw3-dev

# cmake (via pip)
python -m pip install cmake==3.21.4

# use gcc7
update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-7 100
update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-7 100

cd opencv4/
mkdir build-portable/ && cd build-portable/
/usr/local/bin/cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_SHARED_LIBS=OFF -DOPENCV_GENERATE_PKGCONFIG=YES -DOPENCV_FORCE_3RDPARTY_BUILD=YES
make -j5 install

cd $SCRIPT_DIR
mkdir build-portable/ && cd build-portable/
/usr/local/bin/cmake .. -DBUILD_PORTABLE_LINUX=1
make -j5 install

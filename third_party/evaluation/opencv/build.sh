#!/usr/bin/env bash
set -e

mkdir -p build
pushd build && trap 'popd' exit
git clone --depth 1 --branch 4.3.0 https://github.com/opencv/opencv.git
git clone --depth 1 --branch 4.3.0 https://github.com/opencv/opencv_contrib.git
mkdir -p opencv/build
pushd opencv/build && trap 'popd' exit
cmake -D CMAKE_BUILD_TYPE=RELEASE \
        -D INSTALL_C_EXAMPLES=ON \
        -D INSTALL_PYTHON_EXAMPLES=ON \
        -D OPENCV_GENERATE_PKGCONFIG=ON \
        -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
        -D BUILD_EXAMPLES=ON ..
make -j"$(nproc)"
make install

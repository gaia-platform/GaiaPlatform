#!/usr/bin/env bash
set -e

export CC=/usr/bin/clang-8
export CXX=/usr/bin/clang++-8

mkdir -p build
pushd build && trap 'popd' exit
git clone --depth 1 --branch v3.17.3 https://gitlab.kitware.com/cmake/cmake.git
pushd cmake && trap 'popd' exit
./bootstrap
make -j"$(nproc)"
make install

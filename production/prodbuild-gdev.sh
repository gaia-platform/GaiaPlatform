#!/usr/bin/env sh

# This script needs to be run in a gdev-generated docker container.

cmake -DCMAKE_MODULE_PATH=/source/production/cmake /source/production
make -j$(nproc)

(cd /build/production/db/storage_engine && sudo make install)

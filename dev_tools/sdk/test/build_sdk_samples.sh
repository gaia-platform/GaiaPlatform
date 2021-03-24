#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Exit the script as soon as any line fails.
set -e

gaia_db_server &

# Make incubator example.
cp -r /opt/gaia/examples/incubator .
pushd incubator
mkdir build
pushd build
cmake ..
make
popd
popd


# Make and execute Hello World example.
cp -r /opt/gaia/examples/hello .
pushd hello
./build.sh
./run.sh
popd

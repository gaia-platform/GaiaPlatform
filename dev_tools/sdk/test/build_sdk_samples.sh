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
cd incubator
mkdir build
cd build
cmake ..
make
cd /

# Make and execute Hello World example.
cp -r /opt/gaia/examples/hello .
cd hello
./build.sh
./run.sh

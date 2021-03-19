#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set -e

gaia_db_server &

# Make incubator example.
cp -r /opt/gaia/examples/incubator .
cd incubator
mkdir build
cd build 
cmake ..
make 
#./incubator sim 10
cd /

# Make Hello World example.
cp -r /opt/gaia/examples/hello .
cd hello
./build.sh
./run.sh

#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set -e

gaia_db_server &

# Make incubator example.
cd incubator
mkdir build
cd build
cmake ..
make 
cd ../..

# Make Hello World example.
cd hello
mkdir build
cd build
cmake ..
make
cd ../..

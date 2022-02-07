#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Exit the script as soon as any line fails.
echo "Into script."
set -e

# Start the db server.
echo "reset data store."
gaia_db_server --reset-data-store &
echo "sleep."
sleep 1

# Make incubator example.
echo "Create clean inc"
rm -rf ./incubator
cp -r /opt/gaia/examples/incubator .
pushd incubator
echo "Make build"
mkdir build
pushd build
echo "CMake"
cmake ..
echo "Make"
make
popd
popd

# Make and execute Hello World example.
echo "Create clean hello"
rm -rf ./hello
cp -r /opt/gaia/examples/hello .
pushd hello
echo "Build the example."
./build.sh
echo "Run the example."
./run.sh
echo "Example has been run."
popd

# Stop the db server.
pkill -f gaia_db_server

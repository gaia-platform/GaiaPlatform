#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Exit the script as soon as any line fails.
set -e

# Start the db server.
gaia_db_server --reset-data-store &
sleep 1

# Make incubator example.
rm -rf ./incubator
cp -r /opt/gaia/examples/incubator .
pushd incubator
mkdir build
pushd build
cmake ..
make
popd
popd

# Make and execute Hello World example.
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

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

current_dir=$(pwd)

# When this function returns the current directory is where the executable
# is built.
function install_sample {
    cd "$current_dir"
    local sample=$1
    rm -rf ./"$sample"
    cp -r /opt/gaia/examples/"$sample" .
    cd "$sample"
    mkdir build
    cd build
    cmake ..
    make
}

# Make and execute Hello World example. We build this with the provided
# scripts instead of CMake so don't use the 'install-sample' above.
cd $current_dir
rm -rf ./hello
cp -r /opt/gaia/examples/hello .
cd hello
./build.sh
./run.sh

install_sample "incubator"
# We don't run incubator because it is does not have a non-interative mode.

install_sample "direct_access"
./hospital

install_sample "direct_access_vlr"
./direct_access_vlr

install_sample "direct_access_multithread"
./counter

install_sample "rules"
./hospital --non-interactive

install_sample "rules_vlr"
./rules_vlr

# Stop the db server.
pkill -f gaia_db_server

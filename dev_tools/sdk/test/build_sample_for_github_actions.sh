#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Exit the script as soon as any line fails.
echo "Into script."
set -e

# Under normal testing circumstances, we have a fresh install of the SDK and
# the service is already running.  If that is not the case, either start the
# gaia service or start a new instance of the database with something similar
# to:
#
# gaia_db_server --reset-data-store &
#
# OR
#
# sudo systemctl start gaia

if ! pgrep -f "gaia_db_server" > /dev/null 2>&1 ; then
    echo "Database is not currently running.  Please start gaia_db_server and try again."
    exit 1
fi

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

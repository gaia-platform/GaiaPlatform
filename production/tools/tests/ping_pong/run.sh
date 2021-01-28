#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

rm -fr build
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release --log-level=VERBOSE
cd build && make && ./ping_pong
cd ..

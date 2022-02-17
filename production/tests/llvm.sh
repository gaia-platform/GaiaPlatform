#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

pushd /source/production/tools/tests/gaiat || exit
/build/production/llvm/bin/llvm-lit .
popd || exit

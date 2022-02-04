#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

echo "GAIAC"
gaiac -g hello.ddl -d hello -o .

echo "INCLUDE"
CLANG_INCLUDE_PATH="$(clang++-10 -print-resource-dir)/include"

echo "GAIAT"
gaiat hello.ruleset -output hello_ruleset.cpp -- -I "$CLANG_INCLUDE_PATH" -I /opt/gaia/include

echo "CLANG"
clang++-10 hello.cpp hello_ruleset.cpp gaia_hello.cpp /usr/local/lib/libgaia.so -I /opt/gaia/include -Wl,-rpath,/usr/local/lib -lpthread -o hello

echo "DONE"

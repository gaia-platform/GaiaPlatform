#! /usr/bin/bash

#############################################
# Copyright (c) 2021 Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
#############################################

gaiac -g hello.ddl -d hello -o .

CLANG_INCLUDE_PATH="$(clang++-10 -print-resource-dir)/include"
gaiat hello.ruleset -output hello_ruleset.cpp -- -I "$CLANG_INCLUDE_PATH" -I /opt/gaia/include

clang++-10 hello.cpp hello_ruleset.cpp gaia_hello.cpp /usr/local/lib/libgaia.so -I /opt/gaia/include -Wl,-rpath,/usr/local/lib -lpthread -o hello

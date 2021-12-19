#!/bin/bash

#############################################
# Copyright (c) 2021 Gaia Platform LLC
#
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
#############################################

gaiac -g hello.ddl -d hello -o .

gaiat hello.ruleset -output hello_ruleset.cpp -- -I /usr/include/clang/10 -I /opt/gaia/include

clang++-10 hello.cpp hello_ruleset.cpp gaia_hello.cpp /usr/local/lib/libgaia.so -I /opt/gaia/include -Wl,-rpath,/usr/local/lib -lpthread -o hello

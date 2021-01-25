#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

gaiac -g hello.ddl

gaiat hello.ruleset -output hello_ruleset.cpp -- -I /usr/lib/clang/8/include/ -I /opt/gaia/include/

clang++-8 hello.cpp hello_ruleset.cpp /usr/local/lib/libgaia.so -I /opt/gaia/include -Wl,-rpath,/usr/local/lib -lpthread -o hello

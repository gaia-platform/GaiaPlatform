#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

gaiac -g hello.ddl

gaiat hello.ruleset -output hello_rules.cpp -- -I /usr/lib/clang/8/include/ -I /opt/gaia/include/

clang++-8 hello.cpp hello_rules.cpp /usr/local/lib/libgaia.so -I /opt/gaia/include -Wl,-rpath,/usr/local/lib -o hello

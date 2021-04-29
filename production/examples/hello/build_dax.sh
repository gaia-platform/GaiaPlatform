#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

../../build/catalog/gaiac/gaiac -g hello.ddl

../../build/tools/gaia_translate/gaiat hello.ruleset -output hello_ruleset.cpp -- -I /usr/lib/clang/10/include/ -I ../../inc/

clang++-10 hello.cpp hello_ruleset.cpp ../../build/sdk/libgaia.so -I ../../inc -Wl,-rpath,/usr/local/lib -lpthread -o hello

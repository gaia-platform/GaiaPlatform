#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

gaiac -g hello.ddl -d hello -o .

gaiat hello.ruleset -output hello_ruleset.cpp -- -I /usr/lib/clang/10/include -I /opt/gaia/include

clang++-10 hello.cpp hello_ruleset.cpp gaia_hello.cpp /usr/local/lib/libgaia.so -I /opt/gaia/include -Wl,-rpath,/usr/local/lib -lpthread -stdlib=libc++ -o hello

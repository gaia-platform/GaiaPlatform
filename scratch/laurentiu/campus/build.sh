#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

gaiac -g campus.ddl

gaiat campus.ruleset -output campus_rules.cpp -- -I /usr/lib/clang/8/include/ -I /opt/gaia/include/

clang++-8 campus.cpp campus_rules.cpp /usr/local/lib/libgaia.so -I /opt/gaia/include -Wl,-rpath,/usr/local/lib -o campus

# GaiaPlatform
GaiaPlatform - Main repository

## Environment requirements

This repository is meant to be built with clang-8. To ensure clang-8 use, add the following lines to your .bashrc.

* ```export CC=/usr/bin/clang-8```
* ```export CXX=/usr/bin/clang++-8```

## flatbuffers build
The `flatc` (flatbuffer compiler) utility must be built to create headers for some of our programs.
```
cd GaiaPlatform/third_party/production/flatbuffers
mkdir build
cd build
cmake ..
make -j 4
sudo make install
```

## Folder structuring

The following folder structure is recommended for C++ projects:

* inc
  * public
    * component_name (public nterfaces)
  * internal
    * component_name (internal interfaces)
* component_name
  * inc (interfaces that are external for subcomponents, but not for component)
    * sub\_component\_name
  * sub\_component\_name
    * inc (internal interfaces of subcomponent)
    * src
    * tests
  * tests

## Copyright notes

Use the following copyright note with your code. Several language specific versions are provided below.

```
#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------
```


# GaiaPlatform
GaiaPlatform - Main repository

## Environment requirements

This repository is meant to be built with clang-8. To ensure clang-8 use, add the following lines to your .bashrc.

* ```export CC=/usr/bin/clang-8```
* ```export CXX=/usr/bin/clang++-8```

## Folder structuring

The following folder structure is recommended for C++ projects:

* inc (public facing interfaces)
  * component_name
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

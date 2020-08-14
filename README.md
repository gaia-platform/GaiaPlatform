# GaiaPlatform
GaiaPlatform - Main repository

## Environment requirements

This repository is meant to be built with clang-8. To ensure clang-8 use, add the following lines to your .bashrc.

* ```export CC=/usr/bin/clang-8```
* ```export CXX=/usr/bin/clang++-8```

## Folder structuring

The following folder structure is recommended for C++ projects:

* inc
  * internal (global interfaces that are not public)
    * component_name
  * public (global interfaces that are public - these represent the public API)
    * component_name
* component_name
  * inc (interfaces that are external for subcomponents, but not for component)
    * sub\_component\_name
  * sub\_component\_name
    * inc (internal interfaces of subcomponent)
    * src
    * tests (subcomponent level)
  * tests (component level)
* tests (cross-component)

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

## Compile with gdev

Gdev is a command line tool that creates repeatable builds in the GaiaPlatform repo. The builds are
isolated within Docker images and do not depend on any installed packages or configuration on your
host.

Look into `dev_tools/gdev/README.md` to see how to use gdev. 

## Compile locally 

Alternatively to gdev you can compile the project locally. The disadvantage is that you must manually install all the 
dependencies. 

### Install dependencies 

To install all the dependencies go to the `third_party/production/` folder, and follow the instruction of each `gdev.cfg`
file.

Fo instance, let's consider `daemonize/gdev.cfg`:

```text
[apt]
build-essential

[git]
--branch release-1.7.8 https://github.com/bmc/daemonize.git

[run]
cd daemonize
./configure
make -j$(nproc)
make install
```
You will need to run the following commands:

```bash
sudo apt-get install build-essential

git clone --single-branch --branch release-1.7.8 https://github.com/bmc/daemonize.git

cd daemonize
./configure
make -j$(nproc)
make install
cd ..
rm -rf *
```

Same thing for all the other dependencies.

### Build

TO build with Cmake run, from the GaiaPlatform root:

```bash
cmake -S production -B build/production -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"
cd build/production
make -j$(nproc) VERBOSE=1
```

### Run Tests


```
cd build/production

# Run all tests
ctest

# Run specific test
ctest -R storage
```

### Configure Clion

The following steps assumes that you have all the Gaia dependencies installed
locally. If not, go to `GaiaPlatform/third_party/production` and follow the
instructions into each and every `gdev.cfg`.

Note Clion expects a `CMakeLists.txt` at the root of the project. We don't have it. Therefore you need to select 
`GaiaPlatform/production` as root of the project.

1. Open Clion
   - New CMake project from sources
   - Select the `GaiaPlatform/production` directory
2. Build
   - Build ->  Build project
3. Run
   - Run `gaia_se_server` from the top right corner. This should always be running while running other tests.
   - Open a test file (3 time shift and type `test_direct_access.cpp`)
   - Press the play button on the left of each test
4. Modify
   - Add `EXPECT_EQ(true, false)` to any test, press the play button, observe the
     test being compiled, executed and the obvious failure.


# production
This is a folder for production code. Only code that is meant to be shipped should be placed here.

## Build Overview
We have three main sets of targets we build:
1. Core - this includes the core platform components:  catalog, database (client and server), rules engine, extended data classes, and associated tools.
1. SDK - this includes everything in core plus: LLVM libraries, rules translation engine (gaiat), and definitions for building SDK deb and rpm packages.
1. LLVMTests - this includes the LLVM test infrastructure for testing ruleset files as well as ruleset compilation tests.

Because building LLVM takes much longer than just building Core, we do not build it by default.  The SDK does have dependencies on core headers, however, so if you are changing headers for the catalog, database, or direct access components, it is wise to build the entire SDK to ensure you haven't broken the translation engine.

We have two ways to build the configurations listed above:
1. Using docker and our `gdev` tool: this is what our TeamCity Continuous Integration servers use to build and run tests in a consistent "approved" environment.
1. Using your own local environment:  this enables you to build outside of docker using your an environment and tools controlled by you.

The remainder of this document will focus on building in your own local environment outside of a docker container.  

For instructions on how to use `gdev` on your local machine, please see: [gdev docker build CLI](https://github.com/gaia-platform/GaiaPlatform/blob/master/dev_tools/gdev/README.md).

See our `New Hire Guidelines` document on our GaiaPlatform wiki for instructions on how to setup your environment.

The instructions below assume you have created a subfolder **build\** and are executing the following commands in that subfolder.

## Core
```
cmake ..
make -j<number of CPUs>
```
The default CMAKE_BUILD_TYPE is Debug.

## SDK
```
cmake -DCMAKE_MODULE_PATH=/usr/local/lib/cmake/CPackDebHelper -DBUILD_GAIA_RELEASE=ON ..
make -j<number of CPUs>
```
The default CMAKE_BUILD_TYPE is Release.

## LLVMTests
```
cmake -DBUILD_GAIA_LLVM_TESTS=ON ..
make -j<number of CPUs> check-all
```
The default CMAKE_BUILD_TYPE is Release.

## Other Flags
In addition, other CMAKE variables we use but are not required:
```
# To override the build type
-DCMAKE_BUILD_TYPE=Debug|Release
# Get more info on CMake messages
--log-level=VERBOSE
# suppress CMake dev warnings
-Wno-dev
```

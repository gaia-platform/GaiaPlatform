# production
This is a folder for production code. Only code that is meant to be shipped should be placed here.

## Build Overview
We have three main sets of targets we build:
1. Core - this includes the core platform components:  catalog, database (client and server), rules engine, extended data classes, and associated tools.
1. SDK - this includes everything in core plus: LLVM libraries, rules translation engine (gaiat), and definitions for building SDK deb and rpm packages.
1. LLVMTests - this includes the LLVM test infrastructure for testing ruleset files as well as ruleset compilation tests.

Because building LLVM takes much longer than just building Core, we do not build it by default.  However, the SDK does have dependencies on core headers.  If you are changing headers for the catalog, database, or direct access components, it is wise to build the entire SDK to verify that you haven't broken the SDK build.

We have two ways to build the configurations listed above:
1. Using docker and our `gdev` tool: this is what our TeamCity Continuous Integration servers use to build and run tests in a consistent "approved" environment.
1. Using your own local environment:  this enables you to build outside of docker using your own environment and tools.

The remainder of this document will focus on #2:  building in your own local environment outside of a docker container.  For instructions on how to use `gdev` on your local machine, please see: [gdev docker build CLI](https://github.com/gaia-platform/GaiaPlatform/blob/master/dev_tools/gdev/README.md).

For instructions on how to setup your environment, please see our `New Hire Guidelines` document on our GaiaPlatform wiki.

## Build Instructions
Create a subfolder **build\** and then execute the following commands in it depending upon which set of targets you want to build:

### Core
```
cmake ..
make -j<number of CPUs>
```
If CMAKE_BUILD_TYPE is not specified on the command line, then by default we add compile and link flags to include debugging information.

### SDK
```
cmake -DCMAKE_MODULE_PATH=/usr/local/lib/cmake/CPackDebHelper -DBUILD_GAIA_RELEASE=ON ..
make -j<number of CPUs>
```
To install CPackDebHelper, you can follow the steps in the CPackDebHelper [gdev.cfg](https://github.com/gaia-platform/GaiaPlatform/blob/master/third_party/production/CPackDebHelper/gdev.cfg) file. Note that you can specify your own path to the CPackDebHelper cmake module depending upon where you install it.

If CMAKE_BUILD_TYPE is not specified on the command line, then we explicitly set CMAKE_BUILD_TYPE=Release under the covers. This is done by default because debug builds of LLVM take much longer than retail builds.  We've also seen some of our local dev machines run out of memory when attempting to do debug LLVM builds.

### LLVMTests
```
cmake -DBUILD_GAIA_LLVM_TESTS=ON ..
make -j<number of CPUs> check-all
```
If unspecified, the CMAKE_BUILD_TYPE is set to Release as described above for SDK builds.

### Other Flags
Other CMAKE variables we use but are not required:
```
# Override the build type to Debug or Release.
# If explicitly set to Debug, then address sanitizer will be enabled.
-DCMAKE_BUILD_TYPE=Debug|Release

# Get more info on CMake messages.
--log-level=VERBOSE

# Suppress CMake dev warnings.
-Wno-dev
```

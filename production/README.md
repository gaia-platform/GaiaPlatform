# production
This is a folder for production code. Only code that is meant to be shipped should be placed here.

## Build Overview
We have three main sets of targets we build:
1. **Core** - this includes the core platform components:  catalog, database (client and server), rules engine, extended data classes, and associated tools.
1. **SDK** - this includes everything in core plus: LLVM libraries, rules translation engine (gaiat), and definitions for building SDK deb and rpm packages.
1. **LLVMTests** - this includes the LLVM test infrastructure for testing ruleset files as well as ruleset compilation tests.

Because building LLVM takes much longer than just building Core, we do not build it by default.  However, the SDK does have dependencies on core headers.  If you are changing headers for the catalog, database, or Direct Access components, it is wise to build the entire SDK to verify that you haven't broken the SDK build.

We have two ways to build the configurations listed above:
1. Using docker and our `gdev` tool: this is what our TeamCity Continuous Integration servers use to build and run tests in a consistent "approved" environment.
1. Using your own local environment:  this enables you to build outside of docker using your own environment and tools.

The remainder of this document will focus on #2:  building in your own local environment outside of a docker container.  For instructions on how to use `gdev` on your local machine, please see: [gdev docker build CLI README](https://github.com/gaia-platform/GaiaPlatform/blob/master/dev_tools/gdev/README.md).

For instructions on how to setup your environment, please see our *New Hire Guidelines* document on our GaiaPlatform wiki.

## Build Instructions
Create a build folder `BUILD_DIR` anywhere you like and then execute the following commands in it, depending on which set of targets you want to build:

### Core
```
cmake -S GaiaPlatform/production -B $BUILD_DIR
make -j $(nproc)
```
If `CMAKE_BUILD_TYPE` is not specified on the command line, then it is set to `Release` by default. We also default to compiler and linker options that generate enough debugging information to enable readable stack traces, even in `Release` builds.

### SDK
```
cmake -S GaiaPlatform/production -B $BUILD_DIR -DCMAKE_MODULE_PATH=/usr/local/lib/cmake/CPackDebHelper -DBUILD_GAIA_SDK=ON
make -j $(nproc)
```
To install CPackDebHelper, you can follow the steps in the CPackDebHelper [gdev.cfg](https://github.com/gaia-platform/GaiaPlatform/blob/master/third_party/production/CPackDebHelper/gdev.cfg) file. Note that you can specify your own path to the CPackDebHelper `cmake` module depending upon where you install it.

#### Building the distribution packages

After building the SDK, you can also build the distribution packages. To do this, execute the following command in the `GaiaPlatform/production/build` folder:

```
make package
```

After generating the packages, the Debian package can be installed by executing the following command, after making sure to update the referenced package version to match what was produced by the build:

```
sudo apt install ./gaia-0.3.2_amd64.deb
```

(Note that the leading `./` is necessary for `apt` to undrstand that this is a path to a file, not a package name.)

To uninstall the package, execute:

```
sudo apt remove gaia
```

### LLVMTests
```
cmake -S GaiaPlatform/production -B $BUILD_DIR -DENABLE_SDK_TESTS=ON
make -j $(nproc) check-all
```

### Other Flags
Other CMake variables that we use but are not required:

```
# Override the build type to Debug, Release, or RelWithDebInfo (i.e., release-level optimizations with additional debugging information).
# If the build type is Debug, then AddressSanitizer, LeakSanitizer, and UndefinedBehaviorSanitizer will be enabled by default.
-DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo

# Get more info on CMake messages.
--log-level=VERBOSE

# Suppress CMake dev warnings.
-Wno-dev

# Enable verbose build output.
-DCMAKE_VERBOSE_MAKEFILE=ON
```

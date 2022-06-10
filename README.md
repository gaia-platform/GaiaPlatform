# GaiaPlatform

GaiaPlatform - Main repository

## Gaia Platform Overview

For an introduction to Gaia Platform, see: https://gaia-platform.github.io/gaia-platform-docs.io/index.html.

If the above link is not available, the source HTML can also be retrieved from https://github.com/gaia-platform/gaia-platform-docs.io/blob/main/index.html.

## Repository Guidelines

To maintain a healthy codebase, we have a collection of guidelines to use when authoring code.
To enforce that those guidelines are consistantly applied, where possible we use the [Pre-Commit](https://pre-commit.com) application with hooks that match the guidelines.
These guidelines and information on which pre-commit hooks are in place for those guidelines are located in the [Repository Guidelines](docs/repository-guidelines.md) document.
Please review these guidelines before creating or changing any source code within the codebase.

## Environment requirements

The `GAIA_REPO` environment variable is used to refer to the root of the project. Add the following lines to your `.bashrc`:

```bash
export GAIA_REPO=<path_to_your_repoes>/GaiaPlatform
```

This repository is meant to be built with `clang-13`. To ensure `clang-13` use, add the following lines to your `.bashrc`:

```bash
export CC=/usr/bin/clang-13
export CPP=/usr/bin/clang-cpp-13
export CXX=/usr/bin/clang++-13
```

The build system expects the LLVM linker `ld.lld` to be present in your `PATH` and to resolve to the `ld.lld-13` executable. You can ensure this by installing the `lld-13` package on Debian-derived systems (such as Ubuntu) and adding the following line to your `.bashrc`:

```
export LDFLAGS="-B/usr/lib/llvm-13/bin/ -fuse-ld=lld"
```

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

## Compile with gdev

`gdev` is a command line tool that creates repeatable builds in the GaiaPlatform repo. The builds are isolated within Docker images and do not depend on any installed packages or configuration on your host.

Look at the [gdev docker build CLI README](dev_tools/gdev/README.md), to see how to use `gdev`.

## Compile locally

As an alternative to `gdev`, you can compile the project locally. The disadvantage is that you must manually install all the dependencies.

### Install dependencies

Start with the `[apt]` section in [production gdev.cfg](production/gdev.cfg). Install all the packages with `apt install`:

```bash
sudo apt install clang-format-13 clang-tidy-13 debhelper ...
```

Then update the default versions of these commands:

```bash
update-alternatives --install "/usr/bin/clang" "clang" "/usr/bin/clang-13" 10
update-alternatives --install "/usr/bin/clang++" "clang++" "/usr/bin/clang++-13" 10
update-alternatives --install "/usr/bin/ld.lld" "ld.lld" "/usr/bin/ld.lld-13" 10
update-alternatives --install "/usr/bin/clang-format" "clang-format" "/usr/bin/clang-format-13" 10
update-alternatives --install "/usr/bin/clang-tidy" "clang-tidy" "/usr/bin/clang-tidy-13" 10
```

Then move to the `$GAIA_REPO/third_party/production/` folder and follow the instructions in the `gdev.cfg` file within each subdirectory:

For instance, let's consider `daemonize/gdev.cfg`:

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

To build with `cmake`, follow the instructions from the [production README](production/README.md).

### Run Tests

Tests can be run from the build folder with the following commands:

```
cd build/production

# Run all tests
ctest

# Run specific test
ctest -R storage
```

### Configure Clion

The following steps assume that you have all the Gaia dependencies installed locally. If not, go to `third_party/production` and follow the instructions from each and every `gdev.cfg`.

Note that Clion expects a `CMakeLists.txt` at the root of the project. We don't have it. Therefore, you need to select `production` as the root of the project.

1. Open Clion
   - New CMake project from sources.
   - Select the `production` directory.
2. Build
   - Build -> Build project.
3. Run
   - Run `gaia_se_server` from the top right corner. This should always be running while running other tests.
   - Open a test file (3 time shift and type `test_direct_access.cpp`).
   - Press the play button on the left of each test.
4. Modify
   - Add `EXPECT_EQ(true, false)` to any test, press the play button, observe the test being compiled, executed and the obvious failure.

# GaiaPlatform

GaiaPlatform - Main repository

## Style guidelines suggestions for README.md files

* Keep paragraphs on a single line. This makes it easier to update them.
* Use empty lines for separation of titles, paragraphs, examples, etc. They are ignored when rendering the files, but make them easier to read when editing them.
* Use `back-quoting` to emphasize tool names, path names, environment variable names and values, etc. Basically, anything that is closer to coding should be emphasized this way.
* Use **bold** or *italics* for other situations that require emphasis. Bold can be used when introducing new concepts, like **Quantum Build**. Italics could be used when quoting titles of documents, such as *The Art of Programming*. These situations should be rarer.
* Use links to reference other project files like the [production README](https://github.com/gaia-platform/GaiaPlatform/blob/master/production/README.md), for example.

## Environment requirements

This repository is meant to be built with `clang-8`. To ensure `clang-8` use, add the following lines to your `.bashrc`.

```
export CC=/usr/bin/clang-8
export CXX=/usr/bin/clang++-8
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

## Formatter and Linter

### Formatter
`clang-format` is invoked on each commit as a git pre-commit hook. The pre-commit is automatically installed by `cmake`. The `clang-format` version in use is `10.0`.

Note: `clang-format` reorders the includes which could break the build. There are ways to avoid it. Please read: https://stackoverflow.com/questions/37927553/can-clang-format-break-my-code.

### Linter
`clang-tidy` is integrated with `cmake` and is invoked on each build. At the moment, it will only print the warnings in the compiler output. `clang-tidy` is not enforced, which means that warnings do not lead to build failures, keep in mind though, that this is the desired behavior in the long term. Do your best to reduce the number of warnings by either fixing them or by updating the rules in the `.clang-tidy` file. The `clang-tidy` version in use is `10.0`.

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

`gdev` is a command line tool that creates repeatable builds in the GaiaPlatform repo. The builds are isolated within Docker images and do not depend on any installed packages or configuration on your host.

Look at the [gdev docker build CLI README](https://github.com/gaia-platform/GaiaPlatform/blob/master/dev_tools/gdev/README.md), to see how to use `gdev`.

## Compile locally

As an alternative to `gdev`, you can compile the project locally. The disadvantage is that you must manually install all the dependencies.

### Install dependencies

To install all the dependencies, go to the `GaiaPlatform/third_party/production/` folder and then follow the instruction of each `gdev.cfg` file.

For instance, let's consider `daemonize/gdev.cfg`:

```
text
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


```
bash
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

To build with `cmake`, follow the instructions from the [production README](https://github.com/gaia-platform/GaiaPlatform/blob/master/production/README.md).

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

The following steps assume that you have all the Gaia dependencies installed locally. If not, go to `GaiaPlatform/third_party/production` and follow the instructions from each and every `gdev.cfg`.

Note that Clion expects a `CMakeLists.txt` at the root of the project. We don't have it. Therefore you need to select `GaiaPlatform/production` as the root of the project.

1. Open Clion
   - New CMake project from sources.
   - Select the `GaiaPlatform/production` directory.
2. Build
   - Build ->  Build project.
3. Run
   - Run `gaia_se_server` from the top right corner. This should always be running while running other tests.
   - Open a test file (3 time shift and type `test_direct_access.cpp`).
   - Press the play button on the left of each test.
4. Modify
   - Add `EXPECT_EQ(true, false)` to any test, press the play button, observe the test being compiled, executed and the obvious failure.

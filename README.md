# GaiaPlatform

GaiaPlatform - Main repository

## Style guidelines suggestions for README.md files

* Keep paragraphs on a single line. This makes it easier to update them.
* Use empty lines for separation of titles, paragraphs, examples, etc. They are ignored when rendering the files, but make them easier to read when editing them.
* Use `back-quoting` to emphasize tool names, path names, environment variable names and values, etc. Basically, anything that is closer to coding should be emphasized this way.
* Use **bold** or *italics* for other situations that require emphasis. Bold can be used when introducing new concepts, like **Quantum Build**. Italics could be used when quoting titles of documents, such as *The Art of Programming*. These situations should be rarer.
* Use links to reference other project files like the [production README](production/README.md), for example.

## Environment requirements

The `GAIA_REPO` environment variable is used to refer to the root of the project. Add the following lines to your `.bashrc`:

```bash
export GAIA_REPO=<path_to_your_repoes>/GaiaPlatform
```

This repository is meant to be built with `clang-10`. To ensure `clang-10` use, add the following lines to your `.bashrc`:

```bash
export CC=/usr/bin/clang-10
export CPP=/usr/bin/clang-cpp-10
export CXX=/usr/bin/clang++-10
```

(As an aside, the upgrade from `clang-8` to `clang-10` was not driven because we needed features in `clang-10` but rather because the default clang version installed with `apt get install clang` on Ubuntu 20 will install version 10. If you need to continue to build with `clang-8` then that should work just fine.  Please be advised, however, that our TeamCity CI jobs for both Ubuntu 20 and Ubuntu 18.04 will use `clang-10` to build the product that we ship.)

Additionally, the build system expects the LLVM linker `ld.lld` to be present in your `PATH` and to resolve to the `ld.lld-10` executable. You can ensure this by installing the `lld-10` package on Debian-derived systems (such as Ubuntu) and adding the following line to your `.bashrc`:

```
export LDFLAGS=-fuse-ld=lld-10
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

Look at the [gdev docker build CLI README](dev_tools/gdev/README.md), to see how to use `gdev`.

## Compile locally

As an alternative to `gdev`, you can compile the project locally. The disadvantage is that you must manually install all the dependencies.

### Install dependencies

Start with the `[apt]` section in [production gdev.cfg](production/gdev.cfg). Install all the packages with `apt install`:

```bash
sudo apt install clang-format-10 clang-tidy-10 debhelper ...
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

## Release Process

When we are ready to release a new version of Gaia this is the process to follow:

1. Ensure you are on `master` and have the latest changed:
   ```shell
   git checkout master
   git pull 
   ```
2. Bump the project version in the [production/CMakeLists.txt](production/CMakeLists.txt) according to Semantic Versioning 2.0 spec. Note that Major version bumps should involve consultation with a number of folks across the team.
   ```cmake
   # From 
   project(production VERSION 0.2.5)
   # To
   project(production VERSION 0.3.0)
   ```
3. Change, if necessary, the `PRE_RELEASE_IDENTIFIER` in the [production/CMakeLists.txt](production/CMakeLists.txt). For GA releases leave the `PRE_RELEASE_IDENTIFIER` empty.
   ```cmake
   # From  
   set(PRE_RELEASE_IDENTIFIER "alpha")
   # To
   set(PRE_RELEASE_IDENTIFIER "beta")
   ```
4. Create a commit for the new Release:
   ```shell
   git add -u
   git commit -m "Bump version to 0.3.0-beta."
   git push
   ```
5. Create a tag reflecting the new version:
   ```shell
   git tag 0.3.0-beta
   ```
6. Go on [GitHub releases tab](https://github.com/gaia-platform/GaiaPlatform/releases) and draft a new release, using the tag created in the previous step.
   1. Tag Version: `0.3.0-beta`
   2. Release Title: `Gaia Platform 0.3.0-beta`
   3. Description: High level description of new features and relevant bug fixes.
   4. Check the box "This is a pre-release" if that's the case.
8. From now on the version will remain `0.3.0-beta` until a new Release is ready. At that point repeat this process.
   1. We currently have a single version across the product (gaia_sdk, gaia_db_server, gaiac, and gaiat).
   2. Every build has an incremental build number which is added to the full version string (eg. `0.2.1-alpha+1731`). The build number may differ for local builds.

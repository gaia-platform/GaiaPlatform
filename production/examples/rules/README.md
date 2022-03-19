# Rules Demo

A demo of using Gaia Rules.

For more information on Gaie Rules, see [Gaia Rulesets](https://gaia-platform.github.io/gaia-platform-docs.io/articles/rulesets-gaia-rulesets.html) in the Gaia developer documentation.

## Build Instructions

These instructions assume you have installed the SDK and have installed the `clang` and `cmake` tools.  See the SDK User's Guide for instructions on how to do this.

1. To preserve the initial state of the sample code, copy the source files to a new directory.
    ```shell
    mkdir rules
    cd rules
    cp -r /opt/gaia/examples/rules/ .
    ```
2. Create the build directory under the current `rules/` directory and initiate the build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
3. The output of the build is the `rules` executable.

# Running the Demo

To run all of the lessons, run the `rules` binary from the build directory use the following command:

```shell
./rules
```

You can also run lessons one at a time by using the following command:
```shell
./rules <lesson>
```
Where `lesson` must be one of the lessons output from:
```shell
./rules --lessons
```

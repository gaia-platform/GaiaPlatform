# Rules Demo (TODO)

A demo of the Gaia Direct Access API.

For more information on Direct Access, see [Using the Direct Access Classes](https://gaia-platform.github.io/gaia-platform-docs.io/articles/apps-direct-access.html) in the Gaia developer documentation.

## Build Instructions

These instructions assume you have installed the SDK and have installed the `clang` and `cmake` tools.  See the SDK User's Guide for instructions on how to do this.

1. To preserve the initial state of the sample code, copy the source files to a new directory.
    ```shell
    mkdir direct_access
    cd direct_access
    cp -r /opt/gaia/examples/direct_access/* .
    ```
2. Create the build directory under the current `direct_access/` directory and initiate the build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
3. The output of the build is the `hospital` executable.

# Running the Demo

To run the `hospital` binary from the build directory use the following command:

```shell
./hospital
```

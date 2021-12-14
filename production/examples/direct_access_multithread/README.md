# Direct Access Multithread Demo
Demonstrates usage of the Gaia Direct Access API in a multithreaded application.

## Build Instructions
These instructions assume you have installed the SDK and the `clang` and `cmake` tools.  See the SDK User's Guide for instructions.

1. To preserve the initial state of the sample code, copy the source files to a new directory.
    ```shell
    mkdir direct_access_multithread
    cd direct_access_multithread
    cp -r /opt/gaia/examples/direct_access_multithread/* .
    ```
2. Create the build directory under the current `direct_access_multithread/` directory and initiate the build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
3. The output of the build is the `counter` executable.

# Running the Demo

To run the `counter` binary from the build directory, use the following command:

```shell
./counter
```

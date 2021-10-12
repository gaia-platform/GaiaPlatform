# Direct Access Demo
A demo of the Gaia Direct Access API.

## Build Instructions
These instructions assume you have installed the SDK and have installed the `clang` and `cmake` tools.  See the SDK User's Guide for instructions on how to do this.

1. To preserve the initial state of the sample code, copy the source files to a new folder.
    ```shell
    mkdir direct_access
    cd direct_access
    cp -r /opt/gaia/examples/direct_access/* .
    ```
2. Create the build folder under the current `direct_access/` folder and initiate the build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
3. The output of the build is an executable the 'hospital' executable.

# Running the Demo

To run the `hospital` binary from the build folder use the following command:

```shell
./hospital
```

# Direct Access Demo
A demo of the Gaia Direct Access API.

## Build Instructions
These instructions assume you have installed the SDK and have installed the `clang` and `cmake` tools.  See the SDK User's Guide for instructions on how to do this.

1. We suggest copying the sources to a new directory so that you always have a backup in the installed folder if you want to go back.
    ```shell
    mkdir direct_access
    cd direct_access
    cp -r /opt/gaia/examples/direct_access/* .
    ```
2. Create a build directory (in this case it is under the current *incubator/* folder and build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
3. If everything runs successfully the `hospital` executable will be built.

# Running the Demo

You can simply run the `hospital` binary from the build folder:

```shell
./hospital
```

# hello

This is the introductory example for Gaia Platform.

* `hello.ddl` contains the table definitions needed for the example. It gets processed using gaiac.
* `hello.ruleset` contains the example rules. It gets processed using gaiat.
* `hello.cpp` is the main application and gets compiled against the files generated from the other two files.

For additional information about the hello example, see [Writing your first Gaia application](https://gaia-platform.github.io/gaia-platform-docs.io/articles/tutorials/writing-first-gaia-application.html) in the Gaia developer documentation.

## Build Instructions

These instructions assume that you have installed the Gaia SDK, *Clang*, and *CMake*. For information on installing the SDK and tools, see the Gaia SDK [README.md](../../sdk/README.md).

1. To preserve the initial state of the sample code, copy the source files to a new directory: 

    ```bash
    mkdir hello_sample
    cd hello_sample
    cp -r /opt/gaia/examples/hello_sample/* .
    ```

2. To manually build and run the demo:

    ```bash
    ./build.sh
    ./run.sh
    ```

3. To build the demo using the provided *CMake* script:

    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

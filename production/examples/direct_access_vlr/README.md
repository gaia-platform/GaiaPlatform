# Value Linked References with Direct Access example

An example that demonstrates how to use Value Linked References (VLRs) to implicitly connect rows by updating linked fields with Direct Access APIs.

The scenario for this example is about elevators that move between floors in a multi-story office building.

## How VLRs are used in this scenario

The tables `floor` and `elevator` have a 1:N relationship (`floor.elevators -> elevator[]`). Changing the linked field `elevator.floor_number` **implicitly connects** that `elevator` row to a `floor` row whose `floor.floor_number` it matches. It means an elevator has arrived at that floor.

For more information about Value Linked References, see [Implicit Relationships](https://gaia-platform.github.io/gaia-platform-docs.io/articles/reference/ddl-implicit-relationships.html) in the Gaia developer documentation.

## Build instructions

These instructions assume you have installed the SDK and have installed the `clang` and `cmake` tools.  See the [Getting Started guide](https://gaia-platform.github.io/gaia-platform-docs.io/articles/getting-started-with-gaia.html) for instructions on how to do this.

1. To preserve the initial state of the sample code, copy the source files to a new directory.
    ```shell
    mkdir direct_access_vlr
    cd direct_access_vlr
    cp -r /opt/gaia/examples/direct_access_vlr/* .
    ```
2. If it is not already running as a service, start the Gaia Database Server. The persistence flag is optional; if disabled, it will run in-memory instead of storing data on-disk.
    ```shell
    gaia_db_server --persistence disabled &
    ```
3. Create the build directory under the current `direct_access_vlr/` directory and initiate the build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
4. The output of the build is the `direct_access_vlr` executable.

## Running the example

To run the `direct_access_vlr` binary from the build directory use the following command:

```shell
./direct_access_vlr
```

The output will show the logs of an elevator arriving at consecutive floors.

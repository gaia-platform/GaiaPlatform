# Value Linked References with Direct Access example

An example that demonstrates how to use Value Linked References (VLRs) to implicitly connect rows by updating linked fields with Direct Access APIs.

The scenario for this example is a company that occupies a multi-story office building. Each department (Sales, Engineering, etc.) occupies a separate floor of the building. To visit another department, people move between floors.

## How VLRs are used in this scenario

The tables `floor` and `person` have a 1:N relationship (`floor.people -> person[]`). Changing the linked field `person.floor_number` **implicitly connects** that `person` row to a `floor` row whose `floor.floor_number` it matches.

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

The output will show the logs of a person arriving at consecutive floors.

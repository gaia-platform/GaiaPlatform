# Direct Access Value-Linked Relationships Example

An example of using Value-Linked Relationships (VLRs) to implicitly connect rows by updating linked fields with Direct Access APIs.

This example features a scenario of levels in a multi-story office building, each with a different department (Sales, Engineering, etc.) on each level. A person starts in the lobby and moves up floors.

The premise of VLRs: if the tables `level` and `person` have a 1-to-N relationship (`level.people -> person[]`), then changing the linked field `person.level_number` will **implicitly connect** that `person` row to a `level` row whose `level.level_number` matches.

For more information on Value-Linked Relationships, see [Implicit Relationships](https://gaia-platform.github.io/gaia-platform-docs.io/articles/reference/ddl-implicit-relationships.html) in the Gaia developer documentation.

## Build Instructions

These instructions assume you have installed the SDK and have installed the `clang` and `cmake` tools.  See the SDK User's Guide for instructions on how to do this.

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

## Running the Example

To run the `direct_access_vlr` binary from the build directory use the following command:

```shell
./direct_access_vlr
```

The output will show the logs of a person arriving at consecutive levels.

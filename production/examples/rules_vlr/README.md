# Value Linked References in Rules example

An example that demonstrates how to use Value Linked References (VLRs) to implicitly connect rows by updating linked fields in declarative Ruleset code.

This example features college students being automatically assigned to dorm rooms. A declarative Rule selects the first available room for a new student and fills a room until it reaches its resident capacity.

## How VLRs are used in this scenario

The tables `dorm_room` and `student` have a 1:N relationship (`dorm_room.residents -> student[]`). Changing the linked field `student.room_id` **implicitly connects** that `student` row to a `dorm_room` row whose `dorm_room.id` it matches.

For more information about Value Linked References, see [Implicit Relationships](https://gaia-platform.github.io/gaia-platform-docs.io/articles/reference/ddl-implicit-relationships.html) in the Gaia developer documentation.

## Build instructions

These instructions assume you have installed the SDK and have installed the `clang` and `cmake` tools.  See the [Getting Started guide](https://gaia-platform.github.io/gaia-platform-docs.io/articles/getting-started-with-gaia.html) for instructions on how to do this.

1. To preserve the initial state of the sample code, copy the source files to a new directory.
    ```shell
    mkdir rules_vlr
    cd rules_vlr
    cp -r /opt/gaia/examples/rules_vlr/* .
    ```
2. If it is not already running as a service, start the Gaia Database Server. The persistence flag is optional; if disabled, it will run in-memory instead of storing data on-disk.
    ```shell
    gaia_db_server --persistence disabled &
    ```
3. Create the build directory under the current `rules_vlr/` directory and initiate the build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
4. The output of the build is the `rules_vlr` executable.

## Running the example

To run the `rules_vlr` binary from the build directory use the following command:

```shell
./rules_vlr
```

The output will show the logs of students getting assigned to dorm rooms.

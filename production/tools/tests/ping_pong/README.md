# Ping Pong

This is a simple example of high throughput application. The purpose is to test Gaia capabilities with high throughput applications.

There are two actors: Main and Rules. Main initializes the application by writing "pong" in the `ping_pong.status` column. Main then spawns one or more threads that read the value of `ping_pong.status` and, if the value is "pong", will write "ping". This will trigger a rule that checks if the value of `ping_pong.status` is "ping", if so, it writes "pong". This cycle repeats until the application is stopped.

A couple of considerations:
1. The Main workers always check for "pong" and write "ping".
1. The Rule always checks for "ping" and writes "pong".
1. The record updated by Main and Rules is always the same.

The project can be run in two modes:
* Internal: build `ping_pong` against Gaia production
* SDK: build `ping_pong` against the SDK.

# Internal Usage

1. Set `BUILD_PING_PONG_TEST=ON` in Cmake.
1. Run `gaia_db_server` with persistence. Currently, it has to be run manually.
1. Build.
1. Run `./ping_pong [num_workers]`. `num_workers` is optional, if not specified the Main runs with 1 worker.

Note: to increase the chance of seeing problems, increase the Main worker threads or the rules engine threads. Note that running the application in Debug, reduces the frequency of problems.

# SDK usage

The SDK build is independent from the Gaia production project, nothing is inherited from it. To enable the SDK build use ` -DBUILD_SDK=ON`.

1. start gaia_db_server (with persistence enabled):
    ```bash
    sudo service gaia start
    ```
2. Build:
    ```
    cmake  -H. -B.build  -DBUILD_SDK=ON
    cd .build
    make
    ```
1. Run `./ping_pong [num_workers]`. `num_workers` is optional, if not specified the Main runs with 1 worker.

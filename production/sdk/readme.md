# Getting Started with the Gaia SDK

This document provides guidance on setting up the Gaia SDK which, includes the Gaia database server.

## Prerequisites

Before you begin, make sure that you have the following prerequisites:

-   Ubuntu Linux 20.04
-   A machine that supports the x64 architecture.

The Gaia SDK installer installs the Gaia tools and Clang 10.

CMake is the officially supported method for building Gaia applications.  In addition, the Gaia SDK uses CMake to automate code generation from DDL and ruleset files.

To install CMake, run the following command:

`sudo apt-get install cmake`

To build Gaia samples using CMake and make tools, specify the clang compiler by setting the following variables in your environment:

```
export CC=/usr/bin/clang-10
export CPP=/usr/bin/clang-cpp-10
export CXX=/usr/bin/clang++-10
export LDFLAGS="-B/usr/lib/llvm-10/bin/ -fuse-ld=lld"
```

## Download the Gaia SDK

The Gaia SDK is delivered as a Debian software package (DEB).

gaia-x.y.z__amd64.deb

Where x.y.z represents the Gaia version number.

The Gaia SDK includes the Gaia database server executable and the Gaia Declarative C++ SDK.

To download the package, use the time-limited URL that was sent to you in your welcome email.

## Install the package

You must have sudo privileges to install the package.

To install the package:

1. Navigate to the folder that contains the downloaded package.
1.  At the command prompt, replace the x.y.z with the correct version number and run the following commands:

    ```bash
    sudo apt-get update
    sudo apt-get install ./gaia-x.y.z_amd64.deb
    ```

To remove the package:

1. At the command prompt, run the following command:

    ```bash
    sudo apt-get remove gaia
    ```

To update the package, remove it and install the updated package:

1.  Download the updated package.
2.  Navigate to the folder that contains the downloaded package.
3.  To remove the currently installed package, run the following
    command:

    `sudo apt remove gaia`

1.  To install the new version, run the following command after replacing the x.y.z with the version number of the server that you are installing:

    `sudo apt install ./gaia-x.y.x_amd64.deb`

### Installed components

<pre>
/opt/gaia/bin
    gaia_db_server - The Gaia database server.
    gaiac - Gaia Catalog compiler.
    gaiat - Gaia Translation Engine.
/opt/gaia/etc
    gaia.conf - Contains configuration settings for the platform and application loggers that the Gaia Platform uses.
    gaia_log.conf - Contains configuration settings for the database and rules engine that comprise the Gaia Platform.
/opt/gaia/examples/direct_access
    Direct Access Classes example app
/opt/gaia/examples/hello
    Hello World example app
/opt/gaia/examples/incubator
    Incubator example app
/opt/gaia/include
    Include files for the Gaia Platform.
/opt/gaia/lib
    Library files for the Gaia Platform.
</pre>

## Start the Gaia server

The Gaia server must be running to build or run any solution that is based on the Gaia Platform.

We recommend that you don't run gaia\_db\_server under the root user. As with any daemon process that is accessible to the outside, running the Gaia server process as root, or any other account with special access rights, is a security risk. As best practice in production, run Gaia under a separate user account. This user account should only own the data that is managed by the server, and should not be used to run other daemons. For example, using the user `nobody` is not recommended.

To prevent a compromised server process from modifying the Gaia executables, the user account must not own the Gaia executable files.

Gaia server command line arguments:

|   |   |
|---|---|
|--data-dir \<database-folder-path>   | Specifies the location in which to store the Gaia database.  |
|--configuration-file-path \<config-file-name>  | Specifies the location in which to store the Gaia configuration file.  |
| --disable-persistence  | No previous changes to the database will be visible after the database is started, and no changes made while the database is running will be visible after it is restarted.  | 
| --disable-persistence-after-recovery  | Previous changes to the database will be visible after the database is started, but no changes made while the database is running will be visible after it is restarted.  | 
|--reinitialize-persistent-store   |   All previous changes to the database will be deleted from persistent storage and will not be visible after the database is started, but changes made while the database is running will be visible after the database is restarted.  | 

To start the server on a machine that supports systemd:

```bash
sudo apt-get install cmake
```

### Starting the Gaia server on WSL

When starting the Gaia server on WSL, use the --data-dir argument to specify the location in which to store the database. We recommend that you store it locally in ~/.local/gaia/db.

To start the server on Ubuntu and run it in the background on WSL2 (Gaia has not been tested on WSL1):

```bash
gaia_db_server --data-dir ~/.local/gaia/db &
```

## Next Steps

[Gaia Techical Documentation](http://docs.gaiaplatform.io)

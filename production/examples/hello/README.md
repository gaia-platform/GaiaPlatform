# hello

This is the introductory example for Gaia Platform.

* `hello.ddl` contains the table definitions needed for the example. It gets processed using `gaiac`.
* `hello.ruleset` contains the example rules. It gets processed using `gaiat`.
* `hello.cpp` is the main application and gets compiled against the files generated from the other two files.

## Build Instructions
These instructions assume that you have installed the SDK, *clang*, and *cmake*.  For information on installing the SDK and tool, see the SDK [readme.md](../../sdk/readme.md).

1. To preserve the initial state of the sample code, copy the source files to a new directory: 

```script
mkdir hello_sample
cd hello_sample
cp -r /opt/gaia/examples/hello_sample/* .
```

2. To manually build and run the demo:

```script
./build.sh
./run.sh
```

3. To build the and run the dem using the provide *cmake* and *make* scripts:

```script
mkdir build
cd build
cmake ..
make
```

## Inspecting the data
You can use Postgres to inspect the data in the hello database. The `setup.sh` script sets up access to the *hello* example data through Postgres, by creating a *hello* database and importing the Gaia tables into it (this is done using the commands from the `setup.sql` script.) For information on using Postgres with the Gaia SDK, see [Using Postgres to access Gaia database information](https://gaia-platform.github.io/gaia-platform-docs.io/articles/tools/using-postgress-with-gaia.html).

To inspect the data, connect to Postgres as user *postgres*, then execute:

```
\c hello
select * from hello_fdw.names;
select * from hello_fdw.greetings;
```

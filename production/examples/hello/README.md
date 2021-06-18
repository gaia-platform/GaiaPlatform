# hello
This is the introductory example for Gaia Platform.

* `hello.ddl` contains the table definitions needed for the example. It gets processed using `gaiac`.
* `hello.ruleset` contains the example rules. It gets processed using `gaiat`.
* `hello.cpp` is the main application and gets compiled against the files generated from the other two files.

The code can be manually built and run using the commands from shell scripts `build.sh` and `run.sh`. In addition to these, `setup.sh` can be used to setup accessing the *hello* example data through Postgres, by creating a *hello* database and importing the Gaia tables into it (this is done using the commands from the `setup.sql` script).

To inspect the data, connect to Postgres as user *postgres*, then execute:

```
\c hello
select * from hello_fdw.names;
select * from hello_fdw.greetings;
```

You can also build this code with the `cmake` and `make` tools, by using the included CMakeLists.txt file. Before doing this, you should specify the clang compiler by setting the following variables in your environment:

```
export CC=/usr/bin/clang-10
export CXX=/usr/bin/clang++-10
export LD=/usr/bin/ld.lld-10
```

The steps for the cmake build are:

```
mkdir build
cd build
cmake ..
make
```

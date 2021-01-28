# archive/cow_se_demos
This is an archive of old demos based on the initial COW_SE prototype code. They are archived here because they are no longer actively maintained.

The following are the original build instructions:

To build this folder, create a subfolder **build\** and then execute the following commands in it:
* `cmake ..`
* `make`

## cow_se prototype

Implements a prototype database engine that offers snapshot isolation.

Building this project requires installing the Java SDK. Install package *openjdk-8-jdk*.

## airport Q1 demo

Thi `airport_q1` subdirectory contains two C++ programs:
* airimport - loads the airport database into the storage engine for use by other database processors: direct access, SQL, or Gremlin.
* airqueries - queries the airport database. The pre-programmed queries match the Gremlin used in the Q1.

## airport demo

Contains the code for a Python package that provides a Postgresql interface over the cow_se prototype database engine.

### Overview

This code uses Postgres' fdw api with the Multicorn package that maps a row a python dictionary representation to a flatbuffer representation.

The flatbuffer definitions exactly match table specifications. There are extra columns in the schema for the combined graph/relational database model in TrueGraphDb. Every element mapped is a node or an edge in a graph database. Nodes have a unique id (currently 64 bit int, eventually 128 bit guid). Edges have the same kind of unique id and source and dest pointers.

### Instructions

The demo was originally developed on Ubuntu 18.04, but has also been verified to work with 19.10.

1. **Basic tools**
   * Synaptic Package Manager (SPM) - on Ubuntu, this will allow you to install many of the following dependencies.
     * An alternative to Synaptic Package Manager is apt-get, which can be used like this: ```sudo apt-get install <package_name>```.
   * Pip installer is required for installing/removing Python packages. The ubuntu package name is *python-pip*.
   * Python 2.7 - multicorn requires this version.
     * You may need the *python-dev* package in addition to the main package.
   * Pybind11 - needed for wrapping cow_se.
     * Install packages *pybind11-dev* and *python-pybind11*.
   * Clang-8
     * Install packages *clang-8*, *libclang-8-dev*, *libclang-common-8-dev*, *libclang1-8*, *python-clang-8*.
     * Add these lines to your .bashrc:
       * ```export CC=/usr/bin/clang-8```
       * ```export CXX=/usr/bin/clang++-8```
   * Cmake: install the *cmake* package. Unless mentioned otherwise, use the following steps to build a folder that supports cmake (includes a CMakeLists.txt file):
     * ```mkdir build```
     * ```cd build```
     * ```cmake ..```
     * ```make```

2. **cow_se.so** - Python wrapper over the COW prototype storage engine.
   * Build with regular cmake steps under **demos/build/**.

3. **Flatbuffers**
   * Install python flatbuffers package from **third_party/production/flatbuffers/python/** using command ```sudo python setup.py install```.
   * For getting flatc compiler and working with flatbuffers in C++ (these steps are not required for the airport demo):
     * Go to **third_party/production/flatbuffers/**, then build with regular cmake steps.
     * To install flatc compiler and flatbuffers headers and library, execute the following command from the build folder: ```sudo make install```.

4. **Postgres 10.9** (latest 10). This was the default on ubuntu 18. On ubuntu 19 version 10 is not available, so just use version 11.
   * Install *postgresql-10* and *postgresql-server-dev-10* packages using SPM.
   * Start postgres service using: ```sudo service postgresql start```.
   * Test by executing: ```sudo -u postgres psql -c "SELECT version();"```.
   * Helpful Postgres commands:
     * ```\c <database_name>``` - connects to a database.
     * ```\i <script_name>``` - executes a script.
     * ```\dt``` - list tables in current database.
     * ```\dES+``` - list foreign tables in current database.

5. **Multicorn**
   * On ubuntu, there are already available multicorn packages: *postgresql-10-python-multicorn* and *python-multicorn*.

6. **Airport FDW package**
   * Go to **demos/build/ folder** and execute ```make airport```.
   * To quickly test package, go to **demos/airport/tests/** and execute command: ```sudo -u postgres psql -f test_airportdemo.sql```.

7. **Airport demo setup**
   * Decompress and copy the CSV data files from **data/internal/airport/** to tmp folder: ```cp *.dat /tmp```.
   * Go to **demos/airport/setup/** and initialize gaia storage with the command: ```python gaia_init.py```. You have to do this whenever you need to reset the storage.
     * This step can also be executed via ```make gaia-init``` in the **demos/build/** folder.
   * Connect to postgres while in **demos/airport/setup/** folder: ```sudo -u postgres psql```.
   * Load airport data into postgres tables (our storage is not persistent yet): ```\i airportdemo_setup.sql```. You only need to do this once - the *rawdata* tables created by this step can be later reused.
   * You can now create gaia tables and load them up with the data from the Postgres rawdata tables: ```\i airportdemo_gaia_tables_setup.sql```.
   * Play with the data - select from the airports/airlines/routes tables.
     * Sample queries:
       * ```select name from airlines where iata = 'AA';```
       * ```select name from airports where iata = 'CDG';```
       * ```select airlines.name from routes, airlines where src_ap = 'SEA' and dst_ap = 'CDG' and routes.airline=airlines.iata;```
       * ```select distinct ap.name from routes r, airports ap where r.dst_ap = 'SEA' and r.src_ap = ap.iata;```
       * ```select distinct al.name from routes r, airlines al where r.dst_ap = 'SEA' and r.airline = al.iata;```
       * ```select distinct ap.name from routes r, airports ap where r.src_ap = 'SEA' and r.dst_ap = ap.iata;```
       * ```select distinct al.name from routes r, airlines al where r.src_ap = 'SEA' and r.airline = al.iata;```
   * Cleanup gaia tables: ```\i airportdemo_gaia_tables_cleanup.sql```.

## scripts

This folder is meant for storing various demo-related scripts.

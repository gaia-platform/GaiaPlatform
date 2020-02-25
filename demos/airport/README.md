# demos/airport
A demo of using Postgres to query an in-memory database prototype that stores airport/airlines/flights data.

## Overview

This code uses Postgres' fdw api with the Multicorn package that maps a row a python dictionary representation to a flatbuffer representation.

The flatbuffer definitions exactly match table specifications. There are extra columns in the schema for the combined graph/relational database model in TrueGraphDb. Every element mapped is a node or an edge in a graph database. Nodes have a unique id (currently 64 bit int, eventually 128 bit guid). Edges have the same kind of unique id and source and dest pointers.

## Instructions

1. **Basic tools**:
   * Synaptic Package Manager (SPM) - on Ubuntu, this will allow you to install many of the following dependencies.
     * An alternative to Synaptic Package Manager is apt-get, which can be used like this: ```sudo apt-get install <package_name>```.
   * Pip installer is required for installing/removing Python packages. The ubuntu package name is *python-pip*.
   * Python 2.7 - multicorn requires this version.
     * You will need the *python-dev* package in addition to the main package.
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
   * Build with regular cmake steps under demos/build.

3. **Flatbuffers**
   * Get the repository: ```git clone https://github.com/google/flatbuffers.git```, then build with regular cmake steps.
   * To install flatc compiler, execute the following command from the build folder: ```sudo make install```.
   * Install flatbuffers where the global python system can find it, when running as the database. This can be problematic because if you install flatbuffers as yourself, it puts it in ~/.<some hidden pip install>. Install via: ```sudo pip install flatbuffers```. If it complains that you already installed it locally, you have to ```pip uninstall flatbuffers```, then do the sudo install.

4. **Postgres 10.9** (latest 10). This was default on ubuntu 18.
   * On ubuntu, just install *postgresql-10* and *postgresql-server-dev-10* packages using SPM.
   * I (Nick) made life difficult because I’ve had several different postgres on my machine, some parts of the machine think they are on 12, some on 10 which made a few things messy. Doesn’t have to be built from source, but details need elaboration. I had to hack the makefiles because of the v12 vs v10 problem I created. Try to only install one postgres, check if you already have v10. WSL w/Ubuntu 18.04.3LTS has PG 10.10, but I had to install the contrib package before Multicorn would build: sudo apt install postgresql postgresql-contrib.
   * Start postgres service using: ```sudo service postgresql start```.
   * Test by executing: ```sudo -u postgres psql -c "SELECT version();"```.
   * Helpful Postgres commands:
     * ```\c <database_name>``` - connects to a database.
     * ```\i <script_name>``` - executes a script.
     * ```\dt``` - list tables in current database.
     * ```\dES+``` - list foreign tables in current database.

5. **Multicorn**
   * On ubuntu, there are already available multicorn packages: *postgresql-10-python-multicorn* and *python-multicorn*. If you have these available, you don’t need to clone and build multicorn manually. (note that you may need the *python-psutil* package if it’s not installed already).
   * Otherwise, get source code (```git clone git://github.com/Kozea/Multicorn.git```, ```cd Multicorn```; ```make```)
     * Install multicorn extension.

6. **Airport FDW package**
   * In demos/build folder, execute ```make airport```.
   * Old instructions
     * Start in demos/airport/setup.
     * Install to postgres with ```sudo python setup.py install```. (for uninstalling, you can execute: ```sudo pip uninstall airport``` - *airport* is the package name.
       * Note that this package will also include the **cow_se.so** library.
     * Test package using command: ```sudo -u postgres psql -f test_airportdemo.sql``` in demos/airport/tests.

7. **Airport demo setup**
   * Go to data/internal/airport/seattle_only folder and do the following steps:
     * Copy airport data files to tmp folder: ```cp *.txt /tmp```. Note that these only contain the subset of airports and routes that connect directly to SeaTac.
     * Initialize gaia storage: ```python gaia_init.py``` in demos/airport/setup. You can redo this whenever you want to reset the storage.
       * This step can also be executed via ```make gaia-init``` in the demos/build folder.
     * Connect to postgres while in demos/airport/setup folder: ```sudo -u postgres psql```.
     * Load airport data into postgres tables (our storage is not persistent yet): ```\i airportdemo_setup.sql```. You only need to do this once - the *rawdata* tables created by this step can be later reused.
       * If you want a fuller experience, you can load the full airport data using *airport_demo_setup_all.sql* script. Follow the instructions from the script itself.
     * You can now create gaia tables and load them up with the data from the Postgres rawdata tables: ```\i airportdemo_gaia_tables_setup.sql```.
     * Play with the data - select from airports/airlines/routes tables.
       * Sample queries:
         * ```select name from airlines where iata = 'AA';```
         * ```select name from airports where iata = 'CDG';```
         * ```select airlines.name from routes, airlines where src_ap = 'SEA' and dst_ap = 'CDG' and routes.airline=airlines.iata;```
         * ```select distinct ap.name from routes r, airports ap where r.dst_ap = 'SEA' and r.src_ap = ap.iata;```
         * ```select distinct al.name from routes r, airlines al where r.dst_ap = 'SEA' and r.airline = al.iata;```
         * ```select distinct ap.name from routes r, airports ap where r.src_ap = 'SEA' and r.dst_ap = ap.iata;```
         * ```select distinct al.name from routes r, airlines al where r.src_ap = 'SEA' and r.airline = al.iata;```
     * Cleanup gaia tables: ```\i airportdemo_gaia_tables_cleanup.sql```.


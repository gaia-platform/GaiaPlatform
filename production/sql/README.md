# production/sql
This is a folder for the Postgres-based relational engine. The code here defines a foreign data wrapper (FDW) over our database. The FDW allows the gaia database to be used through Postgres.

## Current status

This code is currently being re-written.

The plan is to transition from the static definitions of the airport demo to dynamic querying of the database catalog.

To get there, the code will be rewritten in stages:

* We first define a C++ adapter to isolate the FDW code from the Gaia database internals.
  * During this stage, the adapter will continue to use the static airport code.
* We will next transition from the static airport code to dynamic, catalog-based code, by reimplementing the adapter.
  * THIS IS THE CURRENT STAGE!!!
* At this point, all the old static airport code should be out. Additional work would involve things like adding support for references and array fields.

### Instructions for running unit test

* Edit Postgres login configuration to change postgres account authentication method from "peer" to "trust". To find the location of the configuration file, execute this command in Postgres: `SHOW hba_file;`. By default, the file is located in `/etc/postgresql/12/main/pg_hba.conf`.
* Execute`umask 0` to ensure that `/tmp/gaia_lib/boot_parameters.bin` is writable by all accounts. This will allow it to be accessed by Postgres.
* Run `cmake` with `-DEXECUTE_FDW_TESTS=ON` option.
* After building, install the FDW by running `sudo make install` in the `build/sql/` folder.
* Run tests with `ctest`. It should also execute successfully the `fdw_test.airport` test. The test loads the airport data and performs a few queries, then verifies their results.

# production/sql
This is a folder for the Postgres-based relational engine. The code here defines a foreign data wrapper (FDW) over our database. The FDW allows the gaia database to be used through Postgres.

## Instructions for running unit test

* Edit Postgres login configuration to change local account authentication method from "peer" to "trust". To find the location of the configuration file, execute this command in Postgres: `SHOW hba_file;`. By default, the file is located in `/etc/postgresql/12/main/pg_hba.conf`. You can also edit the file with this command: `sed -i '/^local/ s/peer/trust/' ``pg_conftool -s 12 main show hba_file`` `.
* Execute`umask 0` to ensure that `/tmp/gaia_lib/boot_parameters.bin` is writable by all accounts. This will allow it to be accessed by Postgres. If the file already exists, delete `/tmp/gaia_db/`.
* Run `cmake` with `-DEXECUTE_FDW_TESTS=ON` option.
* After building successfully, install the FDW by running `sudo make install` in the `build/sql/` folder.
* Run tests with `ctest`. It should also execute successfully the `fdw_test.airport` test. The test loads the airport data and performs a few queries, then verifies their results. You can see the airport table and the gaia catalog tables by connecting to Postgres into the 'airport' database and querying tables under the 'airport_fdw' schema. For example, you can execute query `select * from airport_fdw.gaia_table;` to browse the tables defined in the gaia database.

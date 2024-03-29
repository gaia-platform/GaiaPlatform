# production/fdw
This is a folder for the Postgres-based relational engine. The code here defines a foreign data wrapper (FDW) over our database. The FDW allows the gaia database to be used through Postgres.

## Version management

The FDW version is inherited from the production project. The FDW version will consist of just the major/minor combination. The version number is used in the name of the SQL setup script for the FDW (*gaia_fdw--{major}.{minor}.sql*) as well as in the name of the generated library (*gaia_fdw-{major}.{minor}.so*). In the future, we may want to completely drop the version from the library name, but for now, it may still be useful for debugging.

## Instructions for running unit test

* Edit Postgres login configuration to change local account authentication method from "peer" to "trust". To find the location of the configuration file, execute this command in Postgres: `SHOW hba_file;`. By default, the file is located in `/etc/postgresql/12/main/pg_hba.conf`. You can also edit the file with this command: `sed -i '/^local/ s/peer/trust/' ``pg_conftool -s 12 main show hba_file`` `.
* Run `cmake` with `-DEXECUTE_FDW_TESTS=ON` option.
* After building successfully, install the FDW by running `sudo make install` in the `build/fdw/` folder.
* Run tests with `ctest`. It should also execute successfully the `fdw_test.airport` test. The test loads the airport data and performs a few queries, then verifies their results. You can see the airport table and the gaia catalog tables by connecting to Postgres into the 'airport' database and querying tables under the 'airport_fdw' schema. For example, you can execute query `select * from airport_fdw.gaia_table;` to browse the tables defined in the gaia database.

## Handling of NULL values

The FDW accesses data through the flatbuffers reflection API. At this point, the Gaia database doesn't have a uniform way of supporting NULL values. NULL strings can be set through DAC (using our custom `nullable_string_t` class), but this relies on an internal flatbuffers representation that is not consistently handled throughout the flatbuffers API. That representation works in the case of plain string fields, but would not work for string array fields. Another problem is that the reflection API does not allow updating a NULL string value so it cannot be used for that scenario.

Related to this, the reflection API also does not support creating new serializations. This issue was bypassed by programmatically generating *serialization templates* for each table at its creation time - these are serializations in which all fields are set to default values (empty string for strings and zero for numerical fields). These serialization templates are then used whenever an INSERT or UPDATE operation is performed on the table: both operations work by setting the record values in a copy of the serialization template, which is then set or updated in the database record payload.

With this background information, here is the current behavior of the FDW with respect to NULL and in particular, NULL strings:

* Non-reference fields
  * String fields that have been set in the database as NULL will be shown as NULL when read through the FDW.
  * Attempts to set any non-reference field to NULL will act as NO-OP, resulting in the field being set to its default value because of the way the FDW works by updating a serialization template.
  * Attempts to update an existing NULL string value are also avoided because of the way the FDW works by updating a serialization template - we never actually attempt to update a NULL value.
* Reference fields
  * Setting NULL results in removing the reference.
* `gaia_id` field
  * The FDW doesn't allow updating this field, so it cannot be updated to NULL either.

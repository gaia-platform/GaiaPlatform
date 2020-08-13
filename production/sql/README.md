# production/sql
This is a folder for the Postgres-based relational engine.

## Current status

After the client/server separation of the storage engine, the code compiles but the tests do not work.

This code is currently being re-written.

The plan is to transition from the static definitions of the airport demo to dynamic querying of the database catalog.

To get there, the code will be rewritten in stages:

* We first define a C++ adapter to isolate the FDW code from the Gaia database internals.
  * During this stage, the adapter will continue to use the static airport code.
  * THIS IS THE CURRENT STAGE!!!
* We will next transition from the static airport code to dynamic, catalog-based code, by reimplementing the adapter. This will further be broken into two stages:
  * Support for read operations (SELECT).
  * Support for write operations (UPDATE).
* At this point, all the old static airport code should be out. Additional work would involve things like adding support for array fields.

# production/schemas
This folder is used to generate schemas from DDL files. The system schema ddl files should be stored together with the components to which they belong. The contents in the system directory should refer to these DDL file locations. Schemas used for tests should be placed under _schemas/test_.  This way, multiple tests that use the same schema don't have to duplicate the same boilerplate cmake instructions. In an ideal world, only one copy of any given .ddl file (sytem or test) should exist in our entire tree.

Note that all the schema builds are serialized right now so that they don't stomp on each other's database instance if builds are run in parallel (make -jN). This serialization is accomplished simply by having the target of each directory depend on the target before it. Please keep this in mind when adding schema directories.

All schemas are published to **\<build directory\>/schemas/generated**. You can use the variable **GAIA_GENERATED_SCHEMAS** to point to this directory. It is set in the top level production CMake file.

## system
Add schema builds under the relevant component (catalog, rules, etc). See the appropriate CMake file to refer to the target you need to depend upon. Each system schema generates a target called **generate\_\<directory\_name\>\_headers**. For example the generated catalog header has a target named **generate\_catalog\_headers**.

## test
Every test schema generates two targets. The first is the generated header as if the schema were being used in isolation. The second combines all the system schemas and test schema into one "db" header.  The "db" target may become obsolete once we have persistence. Right now it is used to generate headers that are in sync with the system tables in terms of the type identifiers assigned to each table.  Following the convention above, the two targets are:
* **generate\_<directory_name\>\_headers** for standalone
* **generate\_<directory_name\>\_db\_headers** for a "full db" generated header.

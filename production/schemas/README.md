# production/schemas
This folder contains the logic to generate the EDC code from DB schemas. 

System schemas do not have a DDL file, they are generated at runtime during the catalog bootstrap. The generation code uses the database name (eg. `catalog`) to generate the EDC code directly from the Gaia database.

The DDL for the schemas used for tests should be placed under `schemas/test`. This way, multiple tests that use the same schema don't have to duplicate the same boilerplate CMake instructions. In an ideal world, only one copy of any given .ddl file (system or test) should exist in our entire tree.

The generation process creates a static library named after the database we are generating. For instance, the incubator.ddl lead to the creation of a static library named `edc_barn_storage` (since `barn_storage` is the name of the database). The static library exposes as INTERFACE the headers so you can just link it to your target.

You can find the generated code in `\<build directory\>/gaia_generated`. You can use the variable `GAIA_GENERATED_CODE` to point to this directory. It is set in the top level production CMake file.

# Gaia Catalog

This directory contains implementation of Gaia data definition language (DDL)
and catalog.

## API Usage
Including headers in the following directories, and linking `gaia_catalog` is
the most common way to use the catalog library. We recommend use direct access
APIs to navigate and retrieve catalog records.

- `${GAIA_INC}/gaia/db`
- `${GAIA_INC}/internal/catalog`

Linking any sub-components of catalog below are also allowed, and may even be
necessary in certain scenarios. Use it at your own discretion as the
implementation may change more frequently.

## Components

### `src`
This is the source code directory for `gaia_catalog` lib and it contains the
following files.

- `gaia_catalog` implements all the `gaia_catalog` public interfaces.
- `ddl_executor` is a singleton class that implements all the DDL APIs.
- `fbs_generator` implements FlatBuffers schema (fbs) generation APIs.
- `json_generator` implements FlatBuffers default data (json) generation APIs.
- `gaia_generate` implements Gaia extended data classes (EDC) generation APIs.

### `parser`
This is the scanner-parser for Gaia DDL. We used the lex/parser generator--
`flex/bison`--to generate the C++ code for both the lexer and parser. The
lexical analysis rules are defined in [`lexer.ll`](parser/src/lexer.ll). The
grammar rules are defined in [`parser.yy`](parser/src/parser.yy). A helper or
driver class `parser_t` of the lexer-parser is defined in
[`gaia_parser.hpp`](parser/inc/gaia_parser.hpp). Most parsing usage should call
the `gaia::catalog::ddl:parser_t` instead of the generated `yy_lex` or
`yy_parser` interfaces.

Add the following directories to the include list and link `gaia_parser` to use
the parser directly.

- `${GAIA_REPO}/production/catalog/parser/inc`
- `${GAIA_PARSER_GENERATED}`

### `gaiac`
This is the catalog command line tool for Gaia data definition language. It is
used for bootstrapping the EDC definitions, manual testing and development.

The tool has three modes of operation: loading, interactive, and generation.

By default without specifying any mode, `gaiac` will run under loading mode to
execute the DDL statements--translating them into catalog records--without
generating any output.

The interactive mode (`-i`) provides a REPL style command line interface to try
out the DDL. The DDL typed in will be executed, and fbs output if any will be
printed out to the console output.

Under generation mode (`-g`), the tool will generate the following two header
files either from an DDL file or a specified database.

- The FlatBuffers header for field access, `<dbname>_generated.h`
- The EDC header file `gaia_<dbname>.h`

With the two headers, a direct access source file gains access to the database
as defined by the catalog.

#### Usage
```
Usage: gaiac [options] [ddl_file]

  -p          Print parsing trace.
  -s          Print scanning trace.
  -d <dbname> Set the databse name.
  -i          Interactive prompt, as a REPL.
  -g          Generate fbs and gaia headers.
  -o <path>   Set the path to all generated files.
  -t <path>   Start the SE server (for testing purposes).
  -h          Print help information.
  <ddl_file>  Process the DDLs in the file.
              In the absence of <dbname>, the ddl file basename will be used as the database name.
              The database will be created automatically.
```

#### Examples

Enter interactive mode.

```
   gaiac -i

```

Execute DDL statements in `airport.ddl` file using the `airport` database, and
generate header files for tables in `airport` database.

```
   gaiac -g airport.ddl
```

Generate catalog direct access APIs. This is the command used for bootstrapping.

```
   gaiac -d catalog -g
```

## Databases

There are two ways to create a database and specifying a table in a database:
first, using DDL; second, using `gaiac` command. When both are specified, the
DDL definition will override the `gaiac` settings.

### Use DDL

The DDL to create database is `create database`.

To specifying a table in a database, using the composite name of the format
`[database].[table]`.

#### Examples
A database `addr_book` can be created using the following statement.

```
    create database addr_book;
```

Use the following statement to create an `employee` table in `addr_book`
database.

```
create table addr_book.employee (
    name_first: string active,
    name_last: string,
    ssn: string,
    hire_date: int64,
    email: string,
    web: string,
    manages references addr_book.employee
);
```

As a syntactic sugar, the database name can be omitted when specifying a
reference to a table in the same database.

```
    create table addr_book.address (
        street: string,
        apt_suite: string,
        city: string,
        state: string,
        postal: string,
        country: string,
        current: bool,
        addresses references employee
    );
```

### Use `gaiac`

This is the way to create and specify a database before the introduction of the
database to DDL. The `gaiac` command line usage already documented how to do
this. See the following examples for more explanation.

#### Examples
In the following command line example, an `airport` database will be created
automatically. When no database prefix is specified for table names, the tables
will be created in the `airport` database. (The database prefix can still be
used to override the database name derived from the DDL file name.)

```
   gaiac -g airport.ddl

```

The following command line will create all tables in `tmp_airport`. Because a
database name is specified via `-d`, the command will not create the database
`tmp_airport` automatically.

```
   gaiac -d tmp_airport -g airport.ddl

```

## Catalog bootstrapping issue
The ddl executor uses Extended Data Classes to perform operations on the
catalog itself. Compiling the `gaia_catalog` requires header files to define
the EDC classes that are used. This creates a bootstrapping issue where the
original EDC definitions must be generated first. The "code generation"
functions require `gaia_catalog.h` and `catalog_generated.h` to exist before
they can be compiled.

The current solution to this is to run the `gaiac` utility (which calls
functions in the above mentioned files) with `-d catalog -g` to generate the
catalog strictly through the ddl executor API. The catalog initialization
invokes a function called `bootstrap_catalog()`. To build `gaiac` the first
time, a `gaia_catalog.h` was created by hand and `catalog_generated.h` was
generated from a handcrafted fbs definition, sufficient for the code generating
functions to operate. The `gaia_catalog.h` and `catalog_generated.h` in source
control are the ones that were generated by this bootstrapping method.

## How to update the catalog
If the catalog schema is ever updated, it will be necessary to update the
`bootstrap_catalog()` method. After the bootstrap function is updated, `gaiac`
can be built and run again with `-d catalog -g` parameters to generate the new
`gaia_catalog.h` and `catalog_generated.h`. After this, Gaia production should
be rebuilt with the newly generated sources.

Be sure to update
[system_table_types.hpp](../inc/internal/common/system_table_types.hpp) if new
types are added or the `type_id` of the catalog tables change.

Be sure to save the new [gaia_catalog.h](../inc/internal/catalog/gaia_catalog.h)
and [catalog_generated.h](../inc/internal/catalog/catalog_generated.h) in place
of the previous ones.

Be sure to check if [light_catalog.hpp](../db/core/inc/light_catalog.hpp)
or any of the payload FlatBuffers schema files used by it need to be updated.

### Sequence of catalog update steps

* Start by only making necessary catalog schema changes to `bootstrap_catalog()`.
* Build, then execute `gaiac -d catalog -g` (you may need to start the server).
* Copy generated header files (`gaia_catalog.h` and `catalog_generated.h`) under `inc/internal/catalog`.
  * (Optionally) update `light_catalog.hpp` when necessary.
* Now make changes that leverage the schema changes.
* Build again.

## How to add a new system table
The following are the steps to create a new system table with fixed ID:

- Add the table type and its ID to
[system_table_types.hpp](../inc/internal/common/system_table_types.hpp).
- Add the table definition in C++ code to `ddl_executor_t::create_system_tables()`.
- Add build instructions to generate the direct access APIs under
  `${GAIA_REPO}/production/schemas/system`.

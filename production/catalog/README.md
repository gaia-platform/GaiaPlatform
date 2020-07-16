# Gaia Catalog

## Components

`catalog_manager`
This is the catalog manager library.

`parser`
This is the parser library.

`gaiac`
This is an example catalog command line tool for Gaia data definition language.
It is used for manual testing and development.

Currently, when supplied with a file containing Gaia DDL, it will compile it into
the catalog tables, then automatically generate 3 additional files:
- The flatbuffer schema, `<dbname>.fbs`
- The flatbuffer header for field access, `<dbname>_generated.h`
- The EDC header file `gaia_<dbname>.h`

With the two headers, a direct access source file gains access to the database as 
defined by the catalog.

If the `flatc` compiler fails to run during the execution of `gaiac`, the <dbname>_generated.h
file will not be created. To fix this, be sure `flatc` is in PATH and run `gaiac` again.

Alternatively, `flatc` may be run using <dbname>.fbs with the following command-line
arguments:

```
flatc --cpp --gen-mutable --gen-setters --gen-object-api \
  --cpp-str-type gaia::direct_access::nullable_string_t \
  --cpp-str-flex-ctor <dbname>.fbs
```

Usage:
```
   gaiac [-s] [-p] [-i] [ddl_file]
      where: -s ddl_file prints a parsing trace of ddl_file
             -p ddl_file prints a scanning trace of ddl_file
             -i          interactive prompt, as a REPL
             ddl_file    process the DDL without tracing
```
Examples:
```
   gaiac -i
   gaiac airport.ddl
   gaiac -p airport.ddl
   gaiac -s airport.ddl
```

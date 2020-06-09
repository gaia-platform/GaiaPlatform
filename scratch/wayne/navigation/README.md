# Work-in-progress Navigational enhancements to Extended Data Classes

This example has a test program that uses a simple set of airport types to demonstrate
connection and navigation through sets of records.

The gaia_object_t implementation uses a temporary wrapper for the storage engine to
manage an array of pointers for each record. These pointers are each part of connections
to other records in 1:N (parent to child) relationships.

The build process generates two headers from `airport.fbs`. One is `airport_generated.h`
and the other one is `airport_gaia_generated.h`. The second one is ignored because `airport.h`
is used instead. It has been enhanced with templates and additional (to be) generated code.

## Build
The `build` directory is required for this sample:

```
cd build
cmake .. -DGAIA_PROD_BUILD={your path to gaia-platform}/GaiaPlatform/production/build
make
```

Run the program `edc` to see the output.

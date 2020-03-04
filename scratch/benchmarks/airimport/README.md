# airimport
Loads the full data from airlines, airports, and routes into flatbuffers for in-memory use.  Note that right now the data files must be from this project.  

See [../inc/AirportTypes.h](AirportTypes.h) for the type enumeration of the Airlines, Airports, and Routes flatbuffers.

See airport_generated.h for a description of the types generated.  This was generated with the following command:
```
flatc airport.fbs --cpp --gen-object-api --cpp-str-type gaia::string --cpp-str-flex-ctor
```
## build
```
cd <build_dir>
cmake <your git path>/benchmarks/airimport
make
```
## execute
From the build directory above.
```
<build_dir>/airimport ...GaiaPlatform/data/intermal/airport
```

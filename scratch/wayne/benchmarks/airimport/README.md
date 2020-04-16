# airimport
Loads the full data from airlines, airports, and routes into flatbuffers for in-memory use.

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

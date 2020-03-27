# addr_book
An example and performance test for the use of the Storage Engine and gaia_obj_t (Extended Data Class) for accessing the SE.

## build
From this directory:
```
mkdir build
cd build
cmake .. -DFLATC_DIR=<flatc directory>
make
cd ..
```

## run
First form produces timings:
```
build/addr_book ../../../../data/internal/addresses/addresses.csv
```
Second form prints row values:
```
build/addr_book ../../../../data/internal/addresses/addresses.csv print
```

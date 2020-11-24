# Airport Queries

This program was used in the Q1 demonstration to show 5 queries on the airport database.

The program `airqueries` doesn't require any command-line parameters. It requires that `airimport` has been run to load the in-memory database.

Five pre-programmed queries are available. The first 3 are parameterized. To select a query, just provide the menu number.

To use a parameterized query, enter the menu selection followed by comma-separated values, for example:
* `> 1,DFW,OTP` find the 1-layover airports between Dallas-Fort Worth and Bucharest.
* `> 2,LGA,ORD` describe 1-layover routes between La Guardia and O'Hare.

## build

Note that this program uses a specialized version of `edc_object.hpp` named `gaia_object_q1_demo.hpp` in the `airport_q1/inc/` directory.

```
mkdir build
cd build
cmake ..
make
```

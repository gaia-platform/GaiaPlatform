# Incubator Demo
A demo of a rule-based system for controlling the temperature of two incubators.

## Build Instructions
These instructions assume you have installed the SDK and have installed the *clang* and *cmake* tools.  See the SDK User's Guide for instructions on how to do this.

1. To preserve the initial state of the sample code, copy the source files to a new directory.
    ```shell
    mkdir incubator
    cd incubator
    cp -r /opt/gaia/examples/incubator/* .
    ```
2. Create the build directory under the current `incubator/` directory and initiate the build.
    ```shell
    mkdir build
    cd build/
    cmake ..
    make
    ```
3. The output of the build is the 'incubator' executable.

# Running the Demo
The demo operates in two modes:  a show mode and a sim mode.

## Show Mode
This mode simply reads the state of the incubators and outputs it to the console.  This mode is most useful running in a separate console window while the simulator is running.  For example, to dump the state every second, you could run:
```
watch -n 1 ./incubator show
```
This will display something like:
```
-----------------------------------------
chicken |power: ON |min:  99.0|max: 102.0
-----------------------------------------
        |Temp C    |       313|     99.87
        |Temp A    |       313|     99.87
        ---------------------------------
        |Fan A     |       226|    1000.0


-----------------------------------------
puppy   |power: ON |min:  85.0|max: 100.0
-----------------------------------------
        |Temp B    |       313|    100.05
        ---------------------------------
        |Fan C     |       313|    1500.0
        |Fan B     |       313|    1500.0
```
The above shows two incubators, one for chickens and one for puppies.  Both are powered ON.  The chicken incubator is at a temperature of 99.87 degrees.  It contains one fan turning at 1000 rpm.  The fan's last command occurred at timestamp 226.  The simulation has been running for 313 ticks.

## Sim Mode
This mode allows you to control the simulation. The premise is that the incubators are located in a warmer climate such that the temperature will slowly increase if no fans are running.  Each incubator has a minimum and maximum temperature limit.  If the temperature exceeds the limit, then the fans are turned on to cool the incubator.  If the temperature reaches the minimum temperature, then the fans are turned off. To enter simulation mode:
```
./incubator sim
```

This will display a menu:
```
-----------------------------------------
Gaia Incubator

No chickens or puppies were harmed in the
development or presentation of this demo.
-----------------------------------------

(b) | begin simulation
(e) | end simulation
(l) | list rules
(d) | disable rules
(r) | re-enable rules
(p) | print current state
(m) | manage incubators
(q) | quit

main>
```
To run the simulation, type **b** and hit enter.  You can select **p** to print the current state of the incubators or run the incubator in show mode in a separate console window.

In addition to beginning and ending the simulationy ou can also:
* list the subscription information for the rules active in the system
* disable rules by unsubscribing them (all or nothing in the demo)
* re-enable the rules
* manage the incubators - turn them on/off, adjust their temperature settings

Below shows the series of screens and selections to manage the chicken incubator, for example:
```
(b) | begin simulation
(e) | end simulation
(l) | list rules
(d) | disable rules
(r) | re-enable rules
(p) | print current state
(m) | manage incubators
(q) | quit

main> m

(c) | select chicken incubator
(p) | select puppy incubator
(b) | go back

manage incubators> c

(on)    | turn power on
(off)   | turn power off
(min #) | set minimum
(max #) | set maximum
(b)     | go back
(m)     | main menu

chicken>
```

Happy Incubating!

# Troubleshooting
The following are some common errors that can occur when you try to run the demo.

## No output when using /show
### Problem
Running `incubator /show` before running `incubator /sim` for the first time results in no output.  This just means that there is no data loaded into the database for the incubator, sensor, and actuator tables.
```
$ ./incubator show

$
```
### Solution
Run `incubator /sim` at least once to load the simulation in the database.
```
$ ./incubator sim
-----------------------------------------
Gaia Incubator

No chickens or puppies were harmed in the
development or presentation of this demo.
-----------------------------------------

(b) | begin simulation
(e) | end simulation
(l) | list rules
(d) | disable rules
(r) | re-enable rules
(p) | print current state
(m) | manage incubators
(q) | quit

main> q
Exiting...
```
## Connection refuesd
### Problem
The database has not been started yet.
```
terminate called after throwing an instance of 'gaia::common::system_error'
  what():  connect failed - Connection refused
Aborted
```
### Solution
To start the database, restart the gaia service.  For example:
```
systemctl restart gaia
```

## Table (type : N) was not found in the catalog
### Problem
The catalog does not have the incubator schema loaded and therefore couldn't find the incubator types.
```
terminate called after throwing an instance of 'gaia::rules::invalid_subscription'
  what():  Table (type:4) was not found in the catalog.
Aborted
```
### Solution
Do a clean build of your project to load the incubator schema into the database catalog and re-generate headers based on this schema.
```
make clean
make
```
This will populate the catalog with the incubator schema as well as regenerate the required headers.

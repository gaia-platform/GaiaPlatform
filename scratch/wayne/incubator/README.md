# Incubator Demo
This is an alternate implementation of the original incubator demo, contained in REPO/demos/incubator.

## Build Instructions
The Gaia database server, `gaia_db_server` must be running for the build to work. The following example shows how to start the server in such a way that it must be re-initialized each time it is started.

```
gaia_db_server --disable-persistence &
mkdir build
cd build/
cmake ..
make
```

If everything runs successfully the `incubator` executable will be built.

## Initializing the system
After a successful build, the database contains the correct catalog, but needs to have the initial incubator and fan records created. To do this, run:
```
./incubator sim
```

## Re-initializing the database
If you wish to start with a fresh database, the best way is to kill the server, touch and DDL file (incubator.ddl), re-start the server, and re-make the system:
```
pkill gaia_db_server
touch incubator.ddl
make -C build
gaia_db_server --disable-persistence &
```

# Running the Demo
The main program, `incubator`, operates in two modes:  a show mode and a sim mode.

The `sensor` program simulates one thermometer. When it begins, it creates a sensor record and connects it to one of the incubators (as directed on the command-line). Then in a loop that executes once every second, it establishes a new temperature based on the activity of the fan(s). When the thermometer's temperature value is updated, it kicks in rules that control the fans.

Multiple `sensor` programs may be run concurrently, where each one should identify either the "chicken" or "puppy" incubator and the unique name of the sensor (e.g. "TempB").

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

(l) | list rules
(p) | print current state
(m) | manage incubators
(q) | quit

main>
```

Below shows the series of screens and selections to manage the chicken incubator, for example:
```
(l) | list rules
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

## Thermometer Simulation
To start one Thermometer, in a separate command-prompt, run the `sensor` program with the preferred incubator and thermometer name:
```
./sensor chicken TempA
```

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

Happy Incubating!

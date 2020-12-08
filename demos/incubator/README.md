# Incubator Demo
A demo of a rule-based system for controlling the temperature of two incubators.

## Build Instructions
These instructions assume you have installed the SDK and have installed the *clang* and *cmake* tools.  See the SDK User's Guide for instructions on how to do this.

1. We suggest copying the sources to a new directory so that you always have a backup in the installed folder if you want to go back.
```
mkdir incubator_demo
cd incubator_demo
cp -r /opt/gaia/examples/incubator_demo/* .
```
2. Create a build directory (in this case it is under the current *incubator_demo/* folder and build.
```
mkdir build
cd build/
cmake ..
make
```
3. If everything runs successfully the *incubator* executable will be built.

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

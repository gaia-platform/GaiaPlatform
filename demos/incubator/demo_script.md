# Demo Script

## Prep
Ensure that you know where the **incubator.ddl** and **gaia_se_server** executable are located for running the simulation.  The **incubator.ddl** file can be found under */source/demos/incubator* and the **gaia_se_server** process can be found under */build/db/storage_engine*. Start two command/shell windows.  One will show the client and one will show the simulation console.  

## Running
Start client
```
watch -n 1 ./incubator show
```

Start Simulator
```
./incubator sim <path to ddl file> <db server path>
```

## Flow
### Background
1. Recap .ddl file and .ruleset file
1. Point out that the chicken incubator has two sensors and one fan and that the puppy incubator has one sensor and two fans.
1. Discuss that in addition to generating code from the ruleset file, the translation engine also subscribes the rules.
1. Show the rules (talk about how the functions are named now)  Note that we actually generated four functions for three rules because the first rule could be activated by changes either to the sensor table or the incubator table.

### Simulation
1. Start the simulation
1. Do a brief tour of the UI.  Point out the timestamp column as well as column that shows the temperature and current fan speed.

#### First rule
1. Recap the first rule (show the ruleset file)
1. Watch the fans activate as the temperature goes above the maximum temperature range, relate this to the first rule when the sensor.value has changed.
1. Note that the chicken incubator will exceed the temperature first and show the fans turn on due to the temperature getting too high.  The puppy incubator temperature range starts out too high so we'll change it in the simulator console to show that the fans will  turn on either from the sensor value exceeding the maximum threshold or the threshold getting changed.
1. Go to _manage incubators_ and set the puppy temperature threshold down to a value that is less than the current incubator temperature.
1. Wait for temperature to go below the threshold and the fans turn off.

#### Second rule
1. Recap the second rule (show the ruleset file)
1. Get to a point where an incubator has its fans on and the temperature is still above the minimum.
1. Go to the _manage incubators_ select the incubator you want and turn it off.  Note how all the fans are set to 0 immediately.

#### Third rule
1. Recap the third rule (show the ruleset file)
1. Assuming an incubator is off ... wait for the temperature to really exceed the max (by 3 or 4 degrees).  Since the incubator is off, the fans won't turn on.
1. Turn the incubator back on.  The first rule will fire (temp exceeds max) and step the fan by 500 rpm at a time.  However, if the fans get to 70% of max (3500) and the temperature is still too high, then we want to set the fans to their max speed (5000).  You'll see the fans increment from 0 to 3500 by steps of 500 and then jump from 3500 to 5000.

#### Unsubscribe/subscribe
1. Get to a point where one of the incubator has fans that are on and one doesn't
1. Disable the rules
1. The incubator with the fans on will drop below the minimum; the incubator with the fans off will rise above the threshold
1. Re-enabling the rules will show the correct behavior (turn off fans if it's too cool and turn on fans if it's too warm)

## Developer Flow
<WIP>
1. add the following rule to incubator.ruleset:

```
{
    if (incubator.is_on)
    {
        actuator.value = 1500;
    }
}
```

1. Rebuild
1. Re-run and watch on the first power on the actuators will turn-on and then go back to 0

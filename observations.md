# Notes and Observations

## The Mechanics

The heart of the test project is the
[incubator.ddl](https://github.com/gaia-platform/jack-test/blob/master/incubator.ddl)
file.  This simple DDL projects for an `incubator` object with an `is_on` switch,
as well as the maximum (`max_temp`) and minimum (`min_temp`) temperatures for that
incubator.  A temperature sensor is provided (`sensor`) that includes the current
temperature records (`sensor`) and an actuator (`actuator`) with the current speed
of the actuated fan (`value`) in revolutions per minute.

To control this demo, three rules are provided in the
[incubator.ruleset](https://github.com/gaia-platform/jack-test/blob/master/incubator.ruleset)
file.  Rule 2 is the simplest of all rules,
turning off all fans belonging to an incubator is the incubator itself is turned off.
The first and last rules both serve to regulate the temperature in each incubator.
Rule 1 is activated by a change in sensor value, and turns the fan on or off depending
on whether the sensor's `value` field has exceeded `max_temp` on gone below `min_temp`.
Finally, rule 3 is activated if the incubator has exceeded `max_temp` and it currently
is at 70% or more of the fan's current output.

Finally, most of the
[incubator.cpp](https://github.com/gaia-platform/jack-test/blob/master/incubator.cpp)
file is set up to initialize the simulation, to control the simulation, and to report
on the simulation.  During the test, the
[debug mode](https://github.com/gaia-platform/jack-test/blob/master/README.md#debug-mode)
is used to maintain tight control over when any changes occur.  There are two incubators
set up, one for `chicken` and one for `puppy`.  The Chicken Incubator is wired up to
`c_sensor_a` (`Temp A`) and `c_sensor_c` (`Temp C`) along with `c_actuator_a` (`Fan A`).
The Puppy Incubator is wired up to `c_sensor_b` (`Temp B`) along with `c_actuator_b` (`Fan B`)
and `c_actuator_c` (`Fan C`).

This control is enacted by using the
[step command](https://github.com/gaia-platform/jack-test/blob/master/README.md#step-commands).
At the heart of the step command is the
[`simulation_step` method](https://github.com/gaia-platform/jack-test/blob/master/incubator.cpp#L313-L366).
The first `for` loop captures the existing speeds of each of the 3 fans, for use in the next part of
the method.  The second `for` loop calculates the new temperate for each of the 3 sensors.  The `c_sensor_a`
value and `c_sensor_c` value are updated, taking into account the current state of `c_actuator_a`. The
`c_sensor_b` value is updated, taking into account the current states of `c_actuator_b` and `c_actuator_c`.
Each updated value is contained within its own `sensor_writer` instance.

When all actuators and sensors have been visited, a transaction commit is performed.

## Values

90.9999

## Scenario Test 1: "smoke"

- [source location](https://github.com/gaia-platform/jack-test/tree/master/tests/smoke)
- initialize actuators and sensors
- step, pause, and record used (command `z`)
  - step command issued, as documented in above paragraph
  - "pause" occurs until all rules have fired from step
  - database output as JSON blob
- cycle repeated 1023 times

### Chicken Incubator

#### Actual Behavior

Initial settings:
- `min_temp` = 99.0 and `max_temp` = 102.0

Flow:
- timestamp 0 (before reporting)
  - `c_sensor_a` and `c_sensor_c` = 99.0
  - `c_actuator_a` = 0.0
- timestamp 1
  - `c_sensor_a` and `c_sensor_c` = 99.10
  - increments by 0.10 each step with fans off
- lots of steps
- timestamp 30
  - `c_sensor_a` and `c_sensor_c` = 102.0
  - DOES NOT HAPPEN??? - results in the speed for `c_actuator_a` being set to `1000.0` and then `2000.0` before sinking below `max_temp`
  - actual measurement is `101.99995422363281`
- timestamp 31
  - `c_sensor_a` and `c_sensor_c` = 102.1
  - when committed, rule 1 is fired twice, one for each sensor.
  - `c_actuator_a` >= max, so set to 1000.0
- timestamp 32
  - `c_sensor_a` and `c_sensor_c` = 102.07
  - `c_actuator_a` set to 2000.0
- lots of steps
- timestamp 73
  - `c_sensor_a` and `c_sensor_c` = 99.00
  - DOES NOT HAPPEN??? - rule 1 is fired twice (again), reducing speed to 0.00
- timestamp 74
  `c_sensor_a` and `c_sensor_c` = 98.93

#### Expected Behavior

##### Agree
- regardless of starting timestamp, there are two sensors
  - as the temperature calculations are done per sensors, each sensor will be updated with a new temperature
  - Rule 1 will fire once for each sensor change, resulting in fan speed +500 for each sensor
- at timestamp 30, `c_actuator_a` should be set to 1000
  - comparison between `101.99995422363281` and `102.0`
  - 95% sure that the temp will not get above max_temp
  - current measurements indicate that temp equals max_temp for 2 iterations, and exceeds for another 2 iterations
  - talked with Dax about this
    - floating point == comparison is up to the writer of the rule, not Gaia
    - as such, the comparison rightly fails, due to precision

### Puppy Incubator

#### Actual Behavior

Initial settings:
- `min_temp` = 85.0 and `max_temp` = 100.0

Flow:
- timestamp 0 (before reporting)
  - `c_sensor_b` = 85.0
  - `c_actuator_b` and `c_actuator_c` = 0.0
- timestamp 1
  - `c_sensor_b` = 85.20
  - increments by 0.20 each step with fans off
- lots of steps
- timestamp 75
  - `c_sensor_b` = 100.0
  - `c_actuator_b` and `c_actuator_c` remains at 0.0
    - fp issue?
- timestamp 76
  - `c_sensor_b` = 100.20
  - `c_actuator_b` and `c_actuator_c` to 500.0
- timestamp 77
  - `c_sensor_b` = 100.15
  - `c_actuator_b` and `c_actuator_c` to 1000.0
- timestamp 78
  - `c_sensor_b` = 100.10
  - `c_actuator_b` and `c_actuator_c` to 1500.0
- timestamp 79
  - `c_sensor_b` = 100.00
  - `c_actuator_b` and `c_actuator_c` to 1500.0
- lots of steps
- timestamp 229
  - `c_sensor_b` = 85.00
  - `c_actuator_b` and `c_actuator_c` to 500.0
- timestamp 230
  - `c_sensor_b` = 84.95
  - `c_actuator_b` and `c_actuator_c` to 0.0
- lots of steps

#### Expected Behavior

##### Agree
- at timestamp 75, `c_actuator_a` is not set to 500
  - floating point issue
  - value is ever so slightly less than the threshold
- why does the fan rate increase to 1500?
  - [discussion](https://gaiaplatform.slack.com/archives/C016Y112WHW/p1627588084361500)
  - [JIRA](https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1102?atlOrigin=eyJpIjoiYmQ5Nzg5YzY3ZTA0NGFiYjk0MDA0NTQxMzA0YzU1MjkiLCJwIjoiamlyYS1zbGFjay1pbnQifQ)
  - changed, per discussion to
    ```
        if (sensor.value >= incubator.max_temp)
        {
            actuator.value = min(c_fan_speed_limit, actuator.value + c_fan_speed_increment);
            actuator.timestamp = g_timestamp;
        }
        else if (sensor.value <= incubator.min_temp)
        {
            actuator.value = max(0.0f, actuator.value - (2*c_fan_speed_increment));
            actuator.timestamp   = g_timestamp;
        }
    ```
    to
    ```
        if (sensor.value >= incubator.max_temp)
        {
            A:actuator.value = min(c_fan_speed_limit, A.value + c_fan_speed_increment);
            actuator.timestamp = g_timestamp;
        }
        else if (sensor.value <= incubator.min_temp)
        {
            A:actuator.value = max(0.0f, A.value - (2*c_fan_speed_increment));
            actuator.timestamp   = g_timestamp;
        }
    ```
    - timestamp 76+ changed as a result
    - now increments by 500 instead of 1500

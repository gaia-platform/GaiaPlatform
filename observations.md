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

## Starting Point - First Round

This round of observations was performed with the build from 2021 Jun 30.

### Scenario Test 1: "smoke"

- Suite: `smoke`
- [source location](https://github.com/gaia-platform/jack-test/tree/master/tests/smoke)
- initialize actuators and sensors
- step, pause, and record used (command `z`)
  - step command issued, as documented in above paragraph
  - "pause" occurs until all rules have fired from step
  - database output as JSON blob
- cycle repeated 1023 times

#### Chicken Incubator

##### Actual Behavior

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

##### Expected Behavior

###### Agree
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
- throughout the test, and at the end, the entire output should match with no variances
  - see *fluttering* issue below

###### Issues
- occasional *fluttering* in stats
  - filed [JIRA](https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1111)

#### Puppy Incubator

##### Actual Behavior

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

##### Expected Behavior

###### Agree
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
- throughout the test, and at the end, the entire output should match with no variances
  - see *fluttering* issue below

###### Issues

- none, pending above JIRA ticket

### Scenario Test 2: "smoke-time-only"

- Suite: `smoke`
- [source location](https://github.com/gaia-platform/jack-test/tree/master/tests/smoke-time-only)
- initialize actuators and sensors
- step only
  - step command issued, exact same low-level step as in "smoke" scenario
  - no pausing or printing out states
  - literally testing for how quickly can it be processed and on to the next item
- cycle repeated 1023 times

#### Actual Behavior

- tests are processed more quickly than the "smoke" tests due to lack of wait
  - random samply shows 5.2 sec for "smoke" and 0.3 sec for "smoke-time-only"

#### Expected Behavior

##### Agree

- removal of wait after each step should speed up the tests
  - estimated 12x improvement in speed from ~5 sec to ~0.35 sec
- at top level, looks like the number of `scheduled` counts varies quite a lot
  - when looking at the individual totals
    - totals for Rule 1 (`[25]`) and Rule 2 (`[46]`) remain constant at `3072` and `4`
    - the totals for Rule 3 (`[57]`) vary: 227, 255, 217 from 116 in `smoke` case

##### Issues

- based on previous conversations, didn't expect uptick in actuators being turned
  on and off
  - [asked for clarification](https://gaiaplatform.slack.com/archives/C016Y112WHW/p1627674106013200)

### Scenario Test 3: "smoke" with multiple threads

Suite: `threads`, `smoke threads 1` to `smoke threads 4`

#### Actual Behavior

| # Threads | Duration | Total Rule 1 | Retry 1 | Total Rule 2 | Total Rule 3 |
| --- | --- | --- | --- | --- | --- |
| 1 | 5.3 sec | 3072 | 0 | 4 | 118 |
| 2 | 5.1 sec | 3114 | 42 | 4 | 118 |
| 3 | 5.1 sec | 3114 | 42 | 4 | 118 |
| 4 | 5.2 sec | 3114 | 42 | 4 | 118 |

##### Agree

- within tolerance, because of the wait, multiple threads has little influence
  on the duration
- having the extra threads causes a consistent number of retries
- all measured values, except for time, were very consistent, except for slight
  *fluttering*, as seen before

##### Issues

- None

### Scenario Test 4: "smoke-time-only" with multiple threads

#### Actual Behavior

- suite: `threads`, `smoke-time-only threads 1` to `smoke-time-only threads 4`
- multiple values specified in columns document inconsistency

| # Threads | Duration | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |
| 1 | 0.29 sec | 3072 | 0 | 0 | 4 | 236,252,248 | 0 |
| 2 | 0.29 sec | 3253,3228,3236 | 181,156,164 | 0,1,0 | 4 | 311,291,290 | 0 |
| 3 | 0.37 sec | 3313,3335,3329 | 241,263,257 | 1,6,4 | 4 | 327,319,330 | 0,1,2 |
| 4 | 0.66 sec | 3336,3336,3292 | 264,264,220 | 7,6,5 | 4 | 321,312,327 | 2,0,0 |

##### Agree

- in each case, `Total Rule 1` - `Retry 1` equals 3072
- first time seeing a number of exceptions in the output
- duration and number of retries increases with extra threads

##### Issues

- thread starvation? https://gaiaplatform.slack.com/archives/C016Y112WHW/p1627674106013200

### Scenario Test 5: "smoke-time-test" with multiple threads

#### Actual Behavior

- suite: `threads`, `smoke-time-test threads 1` to `smoke-time-test threads 4`
- uses a 1ns pause after each step, instead of no pause as in `smoke-time-only`
- multiple values specified in columns document inconsistency

pause = 1 ns

- too much noise

pause = 10 ns

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- | --- |
| 1 | 0.32 sec | 84.7 | 3072 | 0 | 0 | 4 | 236,216,180 | 0 |
| 2 | 0.37 sec | 84.9 | 3191,3117,3116 | 119,45,94 | 1,0,0 | 4 | 260,190,243 | 0 |
| 3 | 0.42 sec | 85.3 | 3195,3260,3178 | 123,188,106 | 0,0,1 | 4 | 242,282,252 | 0 |
| 4 | 0.43 sec | 85.6 | 3211,3250,3182 | 139,178,110 | 2,4,0 | 4 | 290,307,218 | 0 |

- noise is way to high

pause = 100 ns

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 0.42 sec | 35.5 | 3072 | 0 | 0 | 4 | 122,151,132 | 0 |
| 2 | 0.45 sec | 35.9 | 3228,3132,3123 | 56,60,51 | 0 | 4 | 125,144,152 | 0 |
| 3 | 0.59 sec | 37.3 | 3178,3163,3130 | 106,91,58 | 0 | 4 | 217,219,119 | 2,0,0 |
| 4 | 0.42 sec | 35.5 | 3130 | 58 | 0 | 4 | 121 | 0 |

- noise creeping up

pause = 1000 ns = 1 ms

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 1.46 sec | 7.6 | 3072 | 0 | 0 | 4 | 118 | 0 |
| 2 | 1.42 sec | 8.1 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 3 | 1.43 sec | 8.1 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 4 | 1.44 sec | 8.0 | 3114 | 42 | 0 | 4 | 118 | 0 |

pause = 10000 ns = 10 ms

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 11.6 sec | 1.3 | 3072 | 0 | 0 | 4 | 118 | 0 |
| 2 | 11.6 sec | 1.2 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 3 | 11.7 sec | 1.3 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 4 | 11.6 sec | 1.2 | 3114 | 42 | 0 | 4 | 118 | 0 |

##### Agree


# Notes and Observations

## Version Note

This information was compiled on the daily build from 2021 June 29.
The relevant parts of this process will be repeated with later builds,
and documented.

## Introduction

There are a number of variables that need to be ironed out when talking about
whether or not the system is performant enough for the target application.

The first variable that needs to be addressed is the type of data and the
periodicity with which it arrives.  In the bare "Incubator" example, the
sensor data is calculated and updated every second.  Therefore, the worst
case for this scenario is that all sensors will be updated with new information
every second.

The second variable is whether accuracy is paramount or if speed is paramount.
Using the Incubator example as a baseline, since the sensor value gets updated
with any changes every second, the accuracy is more important than the speed.

## Terminology

The original main loop of the Incubator simulator fired every second to simulate
the updating of sensors.  To isolate and make things reproducible, this was
replaced with a `step` method as documented in the [The Mechanics](#the-mechanics)
section.  As there are various things that the word `iterations` can apply to,
the word `step` is used to specifically refer to iterations of the main sensor
updating loop.

## TL;DR

Go straight to the [Conculsions](#conclusions)

## Observations and Analysis

All observations were made on my laptop.  Partial output from `lscpu`:

```text
Architecture:                    x86_64
CPU(s):                          8
Thread(s) per core:              2
Core(s) per socket:              4
Socket(s):                       1
Model:                           126
Model name:                      Intel(R) Core(TM) i7-1065G7 CPU @ 1.30GHz
CPU MHz:                         2760.971
```

### Foundation

[Scenario Test 1](#scenario-test-1) was the starting point for these measurements.  Using
accuracy as the goal, a simple `wait_for_processing_to_complete` was crafted.
This method used member variables like `g_rule_1_tracker` in each of the rules
to determine when no changes were observed in the firing of the rules.  The
concept of "no changes" was achieved when, following the invocation of the
`step` method, each of the rule trackers showed no change 3 samples in a row
with a 1000 microsecond pause between samples using the `std::this_thread::sleep_for`
method.  After "no change" was achived, the current state of the database was
output as JSON to `output.json`.  In addition, a single thread was used for
the rules engine.

Based on those settings, the required accuracy was maintained.  Tested over
1023 steps of the incubator, the `output.json` file was exactly the same
each time (except for a reported *flutter*, see below).  With extra work, that
pause value was able to be dropped to 100 microseconds, resulting in a drop to
an average of ~1.2 seconds for the 1023 group of changes.  Below that 100
microsecond pause floor, things got confusing.

As illustrated in [Scenario Test 3](#scenario-test-3), the durations were stable, hanging
around the 1 second mark for each batch of rules, or ~1.04 ms per invocation.
This timing included the writing of the print statements, which averaged at
~180 microseconds.

Basically, this means that on my (Jack's) laptop, I was able to get to around
~850 milliseconds per step, with each step being documented and proven to be
accurate.  To reiterate, this was used by using a "self-defined" pause after
each step, to ensure no more rules were firing.  Accuracy of the information
and results was maintained throughout the simulation and the final state of
the simulation was also stable.

### Switching From Accuracy To Speed

With a pivot to speed, [Scenario Test 2](#scenario-test-2), and eventually
[Scenario Test 4](#scenario-test-4),
were conducted.  The purposes of these tests was simple: push the changes
through the system and see what the effects are.  In this set of tests, only
the `step` method was called, with no wait and no state output.  For the
purposes of completness, tests were done to see if there was any consistency
with the final state, and there was not.  Accuracy was not present.

Using a single thread to process the data, all 1023 steps were completed in
290 milliseconds or 283 microseconds per step.  When I increased the number
of threads, the test duration started to creep up and the exceptions started
to show up.  These exceptions were all `transaction_update_conflict` exceptions,
meaning that two transactions were trying to update the same thing.  For
2 threads, the main rule only had 1 exception over the 3 samples, taking 0.29 seconds
in total.  For 3 threads, there were an average of 4 exceptions for the first rule and
1 exception for the last rule, with the duration increasing to 0.37 seconds.
Finally, for 4 threads, the exceptions for the first rule increased to an average
of 6, while the duration increased to 660 millseconds.

Based on this information, excess threads get in the way and cause issues
with the normal processing.  Therefore, the 283 microseconds per step speed
is best achieved using a single thread.

### Middle Ground?

Keeping on the idea of speed, I considered if there was a middle point
where a constant pause could be used in place of a "check and wait" approach.
This caused me to conduct [Scenario Test 5](#scenario-test-5), and eventually
[Scenario Test 7](#scenario-test-7).  To this extent, i was able to find a decent duration
of 440 microseconds when using a constant pause of 200 microseconds.  This gets
the test done without any exceptions

### Improving?

Going back to the original test, I was able to switch to a more fine-grained
approach with [Scenario Test 6](#scenario-test-6) (and
[Scenario Test 7](#scenario-test-7) ), shaving a bit of time off the orignal
measurements.

Seeing as the `sleep_for` method I was working with before gave the thread
scheduler the ability to swap out, its granularity was grounded at 55 microsecond
increments.  By coding a simple `my_sleep_for` method (as opposed to `usleep`),
and reducing the pause time from 100 microseonds to 75 microseconds, without
losing any stability, I was able to drop the execution time from ~850 microseconds
to ~730 microseconds.

## Conclusions

Based on the information I have extracted from the 2021 Jun 29 build:

- data type and periodicity
  - based on what I saw from these trials, the incubator data falls under
    the type of constantly updating or eventually consistent
    - between the natural heating and the fans, always changing within
      short time periods
    - can skip a couple of updates here and there without severe consequences
    - how many we can skip is a sliding scale
  - the other type is enforced consistentcy
    - discrete events happen where dropping any update destroys the consistency
    - must ensure that any updates (insert, delete, update) are acted upon
- accuracy vs speed
  - if accuracy is desired, a pause may be required after each transaction
    - the pause is only required if the velocity of the transactions
      is exceeding ~1 transaction per millisecond
    - using exposed methods and custom code to wait for rules to finish,
      this example hits ~730 ms or 714 microseconds per commit
    - using a simple, constant pause of 200 microseconds, this is dropped to
      ~440 ms or 430 microseconds per commit
      - more experimentation would need to be done to see if larger volumes
        can keep that sustained and scale
  - if speed is desired
    - using a tight loop, that is calculating sensor changes, commiting them
      to to a transaction, and doing it again, the speed achieved was
      290 ms, or 283 microseconds per commit
    - using extra threads caused retries to occur, incuring issues with the
      retrying of transactions and causing exceptions when conflicts occur
      - for this example specifically, using 4 threads doubled the execution
        time

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

### Scenario Test 1

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

### Scenario Test 2

- Suite: `smoke-time-only`
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

### Scenario Test 3

- Suite: `threads`, `smoke threads 1` to `smoke threads 4`
  - specifically `smoke` related tests
- pause before checking to see if all rules have been processed using custom = 100 microseconds
- previous measurement used custom = 1000 microseconds
  - durations were all in the 5 second range
  - behavior for any thread > 1 was consistent
- experimentation found that a custom value of 200 microsecond produced consistent results
  for threads 2+ as well, with only a 0.5 second increase in the execution time
- similar experimentation for 50 microseconds also produced consistent results
  - duration only dropped to 0.89 second, with 2+ produced same results as 2 threads

#### Actual Behavior

| # Threads | Duration | Total Rule 1 | Retry 1 | Total Rule 2 | Total Rule 3 |
| --- | --- | --- | --- | --- | --- |
| 1 | 1.08 sec | 3072 | 0 | 4 | 118 |
| 2 | 1.06 sec | 3114 | 42 | 4 | 118 |
| 3 | 1.12 sec | 3114,3114,3101 | 42,42,9 | 4 | 118,118,93 |
| 4 | 1.10 sec | 3114,3113,3114 | 42,41,42 | 4 | 119,118,118 |

##### Agree

- within tolerance, because of the wait, multiple threads has little influence
  on the duration
- having the extra threads causes a consistent number of retries
- all measured values, except for time, were very consistent, except for slight
  *fluttering*, as seen before

##### Issues

- None

### Scenario Test 4

- suite: `threads`, `smoke-time-only threads 1` to `smoke-time-only threads 4`
  - specifically `smoke-time-only` related tests

#### Actual Behavior

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

### Scenario Test 5

- suite: `threads`, `smoke-time-test threads 1` to `smoke-time-test threads 4`
  - specifically `smoke-time-test` related tests
- uses a 1ns pause after each step, instead of no pause as in `smoke-time-only`

#### Actual Behavior

- multiple values specified in columns document inconsistency

pause = 1 microsecond

- too much noise

pause = 10 microsecond

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- | --- |
| 1 | 0.32 sec | 84.7 | 3072 | 0 | 0 | 4 | 236,216,180 | 0 |
| 2 | 0.37 sec | 84.9 | 3191,3117,3116 | 119,45,94 | 1,0,0 | 4 | 260,190,243 | 0 |
| 3 | 0.42 sec | 85.3 | 3195,3260,3178 | 123,188,106 | 0,0,1 | 4 | 242,282,252 | 0 |
| 4 | 0.43 sec | 85.6 | 3211,3250,3182 | 139,178,110 | 2,4,0 | 4 | 290,307,218 | 0 |

- noise is way to high

pause = 100 microsecond

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 0.42 sec | 35.5 | 3072 | 0 | 0 | 4 | 122,151,132 | 0 |
| 2 | 0.45 sec | 35.9 | 3228,3132,3123 | 56,60,51 | 0 | 4 | 125,144,152 | 0 |
| 3 | 0.59 sec | 37.3 | 3178,3163,3130 | 106,91,58 | 0 | 4 | 217,219,119 | 2,0,0 |
| 4 | 0.42 sec | 35.5 | 3130 | 58 | 0 | 4 | 121 | 0 |

- noise creeping up

pause = 1000 microsecond = 1 ms

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 1.46 sec | 7.6 | 3072 | 0 | 0 | 4 | 118 | 0 |
| 2 | 1.42 sec | 8.1 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 3 | 1.43 sec | 8.1 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 4 | 1.44 sec | 8.0 | 3114 | 42 | 0 | 4 | 118 | 0 |

pause = 10000 microsecond = 10 ms

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 11.6 sec | 1.3 | 3072 | 0 | 0 | 4 | 118 | 0 |
| 2 | 11.6 sec | 1.2 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 3 | 11.7 sec | 1.3 | 3114 | 42 | 0 | 4 | 118 | 0 |
| 4 | 11.6 sec | 1.2 | 3114 | 42 | 0 | 4 | 118 | 0 |

##### Agree

### Scenario Test 6

- suite: `threads`, `smoke threads 1` to `smoke threads 4`
  - specifically `smoke` related tests

- `wait_for_processing_to_complete` replaces `sleep_for` with internal `my_sleep_for`
- standard sleep_for relinquishes control of the thread, seems to be locked at a 55 microsecond reoslution
- internal sleep is usually within 300ns of the targetted sleep time

#### Original Behaviour with Original Pause

| # Threads | Duration | Total Rule 1 | Retry 1 | Total Rule 2 | Total Rule 3 |
| --- | --- | --- | --- | --- | --- |
| 1 | 1.08 sec | 3072 | 0 | 4 | 118 |
| 2 | 1.06 sec | 3114 | 42 | 4 | 118 |
| 3 | 1.12 sec | 3114,3114,3101 | 42,42,9 | 4 | 118,118,93 |
| 4 | 1.10 sec | 3114,3113,3114 | 42,41,42 | 4 | 119,118,118 |

#### XXX

- reduced wait time to 75 microseconds from 100 microseconds

| # Threads | Duration | Total Rule 1 | Retry 1 | Total Rule 2 | Total Rule 3 |
| --- | --- | --- | --- | --- | --- |
| 1 | 0.75 sec | 3072 | 0 | 4 | 118 |
| 2 | 0.75 sec | 3114 | 42 | 4 | 118 |
| 3 | 0.70 sec | 3114,3110,3114 | 42,38,42 | 4 | 118,106,119 |
| 4 | 0.70 sec | 3114 | 42 | 4 | 118 |

### Scenario Test 7

- specifically `suite-scaling` related tests
- suite: `threads`, `smoke-time-test threads 1` to `smoke-time-test threads 4`
  - specifically `smoke-time-test` related tests
- test 5, but with internal `my_sleep_for`

pause = 500 microsecond

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 0.79s | 0.03 | 3072 | 0 | 0 | 4 | 118 | 0 |
| 2 | 0.79s | 0.03 | 3114 | 0 | 0 | 4 | 118 | 0 |
| 3 | 0.83s | 0.04 | 3118,3112,3114 | 46,40,42 | 0 | 4 | 121,118,118 | 0 |
| 4 | 0.84s | 0.04 | 3114 | 0 | 0 | 4 | 118 | 0 |

pause = 100 microsecond

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 0.33s | 0.2 | 3072 | 0 | 0 | 4 | 212,209,214 | 0 |
| 2 | 0.4s | 0.35 | 3128,3128,3172 | 56,56,100 | 0 | 4 | 203,196,229 | 0 |
| 3 | 0.34s | 0.19 | 3126,3130,3124 | 54,58,52 | 0 | 4 | 136,147,141 | 0 |
| 4 | 0.55s | 0.5 | 3256,3121,3142 | 184,49,70 | 5,0,1 | 4 | 260,110,179 | 0 |

pause = 200 microsecond

| # Threads | Duration | % | Total Rule 1 | Retry 1 | Exception 1 | Total Rule 2 | Total Rule 3 | Retry 3 |
| --- | --- | --- | --- | --- | --- | --- |  --- |--- |
| 1 | 0.44s | 0.08 | 3072 | 0 | 0 | 4 | 104,124,99 | 0 |
| 2 | 0.55s | 0.13 | 3129,3131,3129 | 57,59,57 | 0 | 4 | 140,154,141 | 0 |
| 3 | 0.45s | 0.09 | 3131,3130,3130 | 59,58,58 | 0 | 4 | 121 | 0 |
| 4 | 0.54s | 0.17 | 3134,3128,3130 | 62,56,58 | 0 | 4 | 124,124,134 | 0 |

### Scenario Test 8

- specifically `suite-scaling` related tests
  - suite `smoke-time-only` with 100, 1k, 10k, 100k

Series 1

| Iterations | 1 | 2 | 3 | Average |
|---| --- | --- | --- | --- |
| 100 | 0.000278766 | 0.00028581 | 0.000273879 | 280 microseconds |
| 1,000 | 0.000254969 | 0.000353093 | 0.000267675 | 292 microseconds |
| 10,000 | 0.000269671 | 0.00026799 | 0.000273914 | 271 microseconds |
| 100,000 | 0.000266862 | 0.000270386 | 0.000274332 | 271 microseconds |

Series 2

| Iterations | 1 | 2 | 3 | Average |
|---| --- | --- | --- | --- |
| 100 | 0.000524871 | 0.000267696 | 0.000286255 | 360 microseconds |
| 1,000 | 0.000268402 | 0.000274721 | 0.000252329 | 265 microseconds |
| 10,000 | 0.000267622 | 0.000274532 | 0.000271222 | 271 microseconds |
| 100,000 | 0.000271981 | 0.000270047 | 0.000269838 | 271 microseconds |

Series 3

| Iterations | 1 | 2 | 3 | Average |
|---| --- | --- | --- | --- |
| 100 | 0.000254793 | 0.000481039 | 0.00030608 | 347 microseconds |
| 1,000 | 0.000274207 | 0.000284762 | 0.000276683 | 279 microseconds |
| 10,000 | 0.000276676 | 0.000274231 | 0.000274375 | 275 microseconds |
| 100,000 | 0.000268597 | 0.000272426 | 0.000273895 | 272 microseconds |

### Scenario Test 9

- specifically `suite-large-scale` related tests
  - suite `smoke-time-only` with 1m
  - `smoke-time-only-with-1m`

Series 1

| Iterations | 1 | 2 | 3 | Total Average |
|---| --- | --- | --- | --- |
| 1,000,000 | 268.036950807 | rc=5 | rc=5 | 268 microseconds |

Series 2

| Iterations | 1 | 2 | 3 | Total Average |
|---| --- | --- | --- | --- |
| 1,000,000 | 268.036950807 | 338.658779802 | rc=5 | 303 microseconds |

- talked with [Tobin](https://gaiaplatform.slack.com/archives/CNUDLV95J/p1628616593181200)
  - there is an issue with memory management that he is currently working on
  - until that is fixed, not viable to test

# Gaia Platform - Integration Test Suite

As much as possible, this project has been engineered to be copied to
another directory and used for other integration tests, with a minimal
set of changes required.

## TL;DR

If all you want to do is run the tests, and have blind faith in them,
please check out the sections on
[Executing Single Tests](#executing-single-tests) and
[Executing Test Suites](#executing-test-suites)

## Introduction

The main goal of this project is to take a simple example and turn it into
an integration test suite that is predictable, maintainable, portable,
and observable.  Given that statement, if asked why we decided to take
a given route on solving an issue, the explanation will always
focus on one of those four goals.

This framework provides performance and integration tests that can be
part of a CI/CD pipeline.  It is a version of the
[Incubator example](https://github.com/gaia-platform/GaiaPlatform/tree/master/production/examples/incubator)
altered for testing purposes.  Because of the multithreaded nature of
the product, a lot of the testing centers on performance
testing, and that is a big concern.  However, in cases where correctness
is available, the framework provides functionality to help ensure that
a given test executes with predictable results.

## A New Name For The Incubator Application

To that extent, a lot of the changes and commands described in the
following sections are there to provide closer alignment with those goals.
The substantial change is that the "1 second timer" part of the Incubator example
has been amended to include a debug mode.  This debug mode allows for
the frequency between updates to be controlled by the test, instead of
waiting for another second to pass.

After these changes were made, a decision was made to rename the
"Incubator Example" to the "Mink Application".  For more information
on what else changed and why this decision was made, see
[incubator modifications](#modifications-to-the-incubator-example).

## Composition of the Tests

The primary drivers for these tests are built using Bash and Python3.
Please ensure that the result of `python3 --version` is at least
`Python 3.8.10` and that the output of `bash --version` is at least
`GNU bash, version 5.0.17(1)-release`.

In addition, this project requires the standard Gaia requirements
[detailed here](https://github.com/gaia-platform/GaiaPlatform#environment-requirements).

### Note

We tried to mark every `.sh` and `.py` script as executable with the
`git update-index --chmod=+x path/to/file` command.  If we have missed one,
please let us know, and use the `chmod +x path/to/file` on that file to fix
it locally.

## Executing Single Tests

Executing a single test is accomplished by executing the `./test.sh` script.
If executed without any additional commands, the `smoke` test will be
executed.  A full set of available tests can be found by executing
`./test.sh -l`, and a specific test from that list can be executed by executing
`./test.sh <test-name>`.

A common set of arguments to use before the test name are the `-v`/`--verbose`
and the `-vv`/`--very-verbose` flags.   Their behavior is identical for the
`test.sh` script, but the `--very-verbose` flag also passes the `--verbose` flag
to any scripts that it calls.  This can be used to provide more insight to what
the entire system is doing.

### Test Input

High level tests are driven by specifying the `commands.txt` file in a new, unique
directory under `tests`.  This file provides the input required to push the workflow
through the various levels of menus in the mink application.
Additionally, if the end state of the test is something that is expected to be
constant, the optional `expected_output.json` file can be used to validate the output
that is expected from the mink application.

For more information on these files, refer to the following section on
[Creating a Test](#creating-a-test).

In addition, the `config.json` file can be specified at the `tests` level
or the individual test level.  A `config.json` file local to a test will
always supersede the `tests` level `config.json`.  For more information
on this file, refer to the section on
[Gaia Test Configuration](#gaia-test-configuration)

#### Creating a Test

Each test is assigned the unique name of the directory containing the
required `commands.txt` file and the optional `expected_output.json` file.

The `commands.txt` file is a simple file with a single command on each line.
Those commands, except for
[a few added commands](#main-menu-commands),
are the same as the commands for the mink application. For example,
look at the file `smoke\commands.txt`:

```
# Turn chicken portion on.
m
c
on
m
# Turn puppy portion on.
m
p
on
m
# Wait 5 milliseconds to allow the turn on effects to finish.
w5
# Take 1023 steps.
z1023
```

there are two distinct parts of the file, separated by the `w5` command and
the comment preceding it.

If you place this file where you can see it and engage the application in
normal mode in a separate window, you can type each one of these commands
on a line, hit enter, and follow what each one is doing.  The first group
of commands switches to the incubator menu (`m`), selects chickens (`c`),
enabled the chicken incubator (`on`), and then returns to the main menu (`m`).
The second group of commands performs the same action, but on the puppy (`p`)
incubator instead.

At that point, the second part of the file takes over.  This is the part of
the file that uses the
[incubator modifications](#modifications-to-the-incubator-example)
to more accurately control the application's behavior.  To start with,
the `w5`command waits for 5 milliseconds to ensure that any rules triggered
by the setup will complete before the testing continues.  This may not be
100% necessary, but it provides a good buffer to ensure the isolation of
test measurements.

With that pause completed, the test then invokes the `step_and_emit_state` function `1023` times
before exiting the simulation.  As detailed in the section on
[incubator modifications](#modifications-to-the-incubator-example),
a step is a single loop of processing for the simulation. In particular,
the `step_and_emit_state` method invokes the step, waits until any rules
from the Rules Engine have fired, and then outputs the step.  This allows
a picture of the database along an entire run of the test to be collected
and compared to the `expected_output.json` file for that test.

Note that due to the multithreaded nature of Gaia, specific circumstances
must be in place for the workflow to be predictable. With the Mink Application,
the only way to ensure predictability is to use a single thread and to
verify that all rules have executed before taking the next step.  With
every other setting, the *random* elements associated with multithreading
will not result in a predictable outcome.

### Test Output

Upon completion of the test run, the results of that test run are copied into
the `test-results` directory.
While it is not a complete copy of everything that was used to execute the test,
it is a copy of all relevant input files and all relevant files produced by
that test.  Note that at the beginning of each test run, this directory is
deleted.

Common files to see in this directory are:
- `duration.json`
  - JSON output file containing the duration of the test
  - captured around the `run.sh` script using `date +%s.%N`
- `expected.diff`
  - simple file created by `diff`ing the test's optional `expected_output.json` file with the generated `output.json` file
- `gaia.log`
  - clean version of the log file generated by the rule engine
- `gaia_stats.log`
  - clean version of the log file generated by the rule engine
- `mink.conf`
  - configuration file generated from the `config.json` file
- `output.csv`
  - experimental, for output to spreadsheets
  - various test measurements, mostly various time-based delays, in CSV format
- `output.delay`
 - various test measurements, mostly various time-based delays, in JSON format
- `output.json`
  - output file generated by executing the commands
- `output.txt`
  - log of the results of the `test.sh` script in `-v`/verbose mode
- `return_code.json`
  - simple file with the return code emitted by the test

## Test Return Codes

A given test can have one of a small set of return codes.  A return code
of `0` means that the tests passed with no errors.  For the other return
codes:

- `1` - some part of the setup of the suite itself did not complete properly
- `2` - a failure occurred during the setup or the teardown of the test itself
- `5` - the underlying invocation of the `run.sh` script failed
- `6` - the test completed, but the data in an existing `expected-output.json` file did not match the actual output

## Executing Test Suites

Executing a test suite is accomplished by executing the `./suite.sh` script.
If executed without any additional commands, the `smoke` suite will be executed.
A full set of available suites can be found by executing
`./suite.sh -l`, and a specific suite can be executed by executing
`./suite.sh <suite-name>`.

### Suite Input

Like test input, the suite input occurs under the `tests` directory.
However, the input here are text files named `suite-<suite-name>.txt`.

In the simplest form, each line contains the name of a single test to execute,
by name.  For example, the `suite-smoke.txt` contains:

```
smoke
smoke-time-only
```

This instructs the `smoke` suite to execute the tests `smoke` and `smoke-time-only`,
in that order, and then exit.

#### Readability

To make sure that these files are readable and maintainable, blank lines and line
comments are supported.  Both will not affect tests. It is recommended that
scenario tests start with some manner of prolog like:

```text
#
# Simple test suite to ...
#
# This test ...

```

The *title* should be expanded on in the first line to provide a more expansive,
but still simple, description of the test.  If there is still some ambiguity about
what the test does or information that can help the reader, the `this test...`
paragraph should explain the test as concisely as possible.

#### Specifying Repeats For A Test

In specific cases, there is a need to run a given test multiple times within a suite.
When that is required, the following format is required:

```
smoke repeat 5
```

That line with execute the `smoke` test 5 times as part of the current suite.

#### Specifying Threads For A Test

By default, the [Gaia Test Configuration File](#gaia-test-configuration) specifies
the number of threads to use when executing any of the tests.  On a per-test
basis, this can be changed to provide comparisons for a given workload with
a changing thread count.

For example, part of the `suite-threads.txt` file looks like this:

```text
smoke threads 1 repeat 3
smoke threads 2 repeat 3
smoke threads 3 repeat 3
smoke threads 4 repeat 3
```

This instructs the `suite.sh` script to execute the `smoke` test 3 times,
one group for a thread count between `1` and `4`.

Note that if specifying the `threads` modifier and `repeat` modifier for
the same test, the `threads` modifier must come before the `repeat` modifier,
and be separated from each other by at least one whitespace character.

#### Creating a Test Suite

Creating a test suite is easy.  Following the naming convention
of `suite-<suite-name>.txt` specified above, create a new text file, and
populate it with one or more test names.  If desired, add the `repeat <n>`
clause after a given test to execute it multiple times.  As the filename
within the directory is guaranteed to be unique, the suite name is guaranteed
to be unique.

### Suite Output

All suite output is stored in the `suite-results` directory.  This directory
is cleared at the start of each suite test run.

Each execution of the suite generates 2 files: `build.txt` and `summary.json`.
As the suite is executing multiple times on the same code base, the code base
is installed in the test directory and built once, to speed up the suite tests.
The results of that single build are stored in the `build.txt` file.

At the end of the suite of tests, the `summary.json` is created.  For every
test that was specified in the input file, a single entry with that test's
name is present in the JSON blob.  If the `iterations` key under that
name JSON blob and its value is a single integer, then a single test was executed
to produce the results.  If the `iterations` key instead contains an array
of values, then the `repeat` modifier was used.

Along the way, the results of each test's `test-result` directory are persisted
in the `suite-results` directory.  The directory containing those results for
a simple test are just the name of the specified test.  For the example in
the section
[Suite Input](#suite-input),
two directories would be created: `test-result/smoke` and `test-result/smoke-time-only`.
For repeat tests, a similar pattern follows, but the directory names are appended
with the number of the iteration of the test.  For the example in the section
[Repeating A Test](#repeating-a-test),
this means that 5 directories would be created, `test-result/smoke_1` to
`test-result/smoke_5`.

### Suite Isolation

To try and ensure that the timing results from one test within a suite does not
affect another test within the same suite, suite tests are executed with a
15 second pause before each test.  Currently, this is not configurable.  While
15 seconds may seem like a long time, it is worth it to ensure that isolate between
the various test suites is maintained.

To be clear with any readers, this 15 second pause is my (Jack's) entirely paranoid
pause.  Its purpose is to ensure that operating system level processing and any
leftover test processing from any previous tests is extinguished before the next test
begins.  There are many things
that the test framework cannot control: other applications, running in a VM versus
bare metal, a slack message coming to me during the middle of tests.  Each of these
will influence any performance measurements.  That is why during any normal
measurements, multiple tests are executed and the averages are taken.

The 15 second pause may indeed be paranoid, but it is a paranoid decision that I
can easily defend.  It may be more time than is needed, but I am confident that
it is at least enough time (under 98% of situations) to let the system cool down
before another test.

### Suite Summaries

Unless the `suite.sh` script is started with the `-n`/`--no-stats` flag, the script will print out a summary of each test in the suite that was executed.

### Integration Summaries

For any tests that are not executed with the `repeat` keyword text is output that looks like the following:

```text
---
Test: smoke
---
  Source        /tests/suite-smoke.txt:5
  Test Type     integration
  Test Results  Success
```

For any tests that are executed with the `repeat` keyword text is output that looks like the following:

```text
---
Test: smoke
---
  Source               /tests/suite-repeat.txt:2
  Test Type            integration
  Test Summary         Success=3/3
  Test Duration (sec)  ['1.988', '2.008', '2.003']
```

### Performance Summaries

As the granularity of most measurements is in nanoseconds, unless explicitly stated, all measurements are presented in microseconds.  This was decided on to allow for the values to line up nicely, with a standard 3 digits to the right of the decimal place.

For any tests that are not executed with the `repeat` keyword text is output that looks like the following:

```text
---
Test: smoke-time-only
---
  Source            /tests/suite-smoke.txt:6
  Steps/Iterations  1023
  Test Type         performance
  Test Results      Success

    Measure         Value
    ----------  ---------
    iter-total  314.721µs
    start        52.436µs
    inside      159.048µs
    commit      102.958µs
    update-row   21.277µs
    wait          0.000µs
    print         0.000µs

    Measure           Value
    -----------  ----------
    avg-latency   270.000µs
    max-latency  1540.000µs
    avg-exec       10.000µs
    max-exec      150.000µs
```

For any tests that are executed with the `repeat` keyword text is output that looks like the following:

```text
---
Test: smoke-time-only-with-100k
---
  Source            /tests/suite-scaling.txt:10
  Steps/Iterations  100000
  Test Type         performance
  Test Results      Success=3/3

    Measure           Min        Max    Range  TP50 (Med)       TP90
    ----------  ---------  ---------  -------  ----------  ---------
    iter-total  271.942µs  272.415µs  0.473µs   272.060µs  272.344µs
    start        48.423µs   48.868µs  0.445µs    48.496µs   48.794µs
    inside      143.811µs  144.285µs  0.474µs   144.184µs  144.265µs
    commit       78.916µs   79.557µs  0.641µs    79.153µs   79.476µs
    update-row   19.714µs   20.010µs  0.296µs    19.760µs   19.960µs
    wait          0.000µs    0.000µs  0.000µs     0.000µs    0.000µs
    print         0.000µs    0.000µs  0.000µs     0.000µs    0.000µs

    Measure             Min         Max       Range  TP50 (Med)        TP90
    -----------  ----------  ----------  ----------  ----------  ----------
    avg-latency   265.458µs   292.546µs    27.088µs   286.386µs   291.314µs
    max-latency  2140.000µs  9970.000µs  7830.000µs  3390.000µs  8654.000µs
    avg-exec       10.000µs    10.000µs     0.000µs    10.000µs    10.000µs
    max-exec      430.000µs   590.000µs   160.000µs   580.000µs   588.000µs
```

All this information is available is raw form in the `summary.json` file detailed in the [section](#summary.json-file) describing the file, but is aggregated for simplicity.

#### Available Measures

For all tests, the following general rows are reported.  More information on these fields are available [in this section](#single-test-json-blob).

- `iter-total`
  - aggregation of the `iteration-measured-duration-sec` file from `summary.json`
- `start`
  - aggregation of the `average-start-transaction-microsec` file from `summary.json`
- `inside`
  - aggregation of the `average-inside-transaction-microsec` file from `summary.json`
- `commit`
  - aggregation of the `average-end-transaction-microsec` file from `summary.json`
- `update-row`
  - aggregation of the `average-update-row-microsec` file from `summary.json`
- `wait`
  - aggregation of the `average-wait-microsec` file from `summary.json`
- `print`
  - aggregation of the `average-print-microsec` file from `summary.json`

- `avg-latency`
  - aggregation of the `rules-engine-stats.calculations.avg-latency-ms` file from `summary.json`
- `max-latency`
  - aggregation of the `rules-engine-stats.calculations.max-latency-ms` file from `summary.json`
- `avg-exec`
  - aggregation of the `rules-engine-stats.calculations.avg-exec-ms` file from `summary.json`
- `max-exec`
  - aggregation of the `rules-engine-stats.calculations.max-exec-ms` file from `summary.json`

#### Available Measure Aggregations

For tests that were repeated, the following columns are reported:

- `min`
  - the minimum value over all samples
- `max`
  - the maximum value over all samples
- `range`
  - the range between the `min` value and the `max` value
- `TP50 (Med)`
  - percentile 50% measurement
- `TP90`
  - percentile 90% measurement

### Summary.json File

The elements in the `summary.json` file are organized by the name of the test that
they are executing.  Regardless of whether it is a single test or a group of tests, that first
element is always the name of that test.  After that, there is some divergence
on how the information is stored to more efficiently aggregate the information from a
group of tests.

#### JSON Blob Naming

In most cases, each entry in the specified `suite-*.txt` file will
have a unique name.  When the information for that entry is placed in the `summary.json`
file, the name associated with the blob is the name of the test that was executed.
Therefore, if the specified `suite-*.txt` file is:

```text
smoke
smoke-time-only
```

it is expected that the `summary.json` file will contain one summary named `smoke`
that contains the information for the first test and another summary named `smoke-time-only`
that contains the information for the second test.

To handle the rare cases where there are duplicates, the names of the blobs are
adjusted to avoid collisions.  Depending on the type of blob being added, one or more
underscore characters (`_`) are appended to the test name, followed by an index number
to ensure that the name is unique.  To ensure that there is a direct correlation between
a line in the `suite-*.txt` file and a specified blob, the `source` blob inside of the
test blob identifies the exact source file and line number that generated the blob.

#### Single Test JSON Blob

For a single test, for example the `smoke` test, the output will look something
like the following example.  Note that the information in the `slices` blob has been
omitted as most of that information is aggregated in other places within the
overall JSON blob.

```json
"smoke": {
  "source": {
      "file_name": "/../tests/suite-something.txt",
      "line_number": 1
  },
  "configuration": {
      "thread_pool_count": 1,
      "stats_log_interval": 2,
      "log_individual_rule_stats": true,
      "rule_retry_count": 3
  },
  "iterations": 1023,
  "return-code": 0,
  "duration-sec": 11.230913584,
  "stop-pause-sec": 6.000195129,
  "wait-pause-sec": 4.435873457,
  "print-duration-sec": 0.326038276,
  "start-transaction-sec": 0.046970413,
  "inside-transaction-sec": 0.145943434,
  "end-transaction-sec": 0.07774411,
  "update-row-sec": 0.016623276,
  "average-start-transaction-microsec": 32.230933529,
  "average-inside-transaction-microsec": 120.894697947,
  "average-end-transaction-microsec": 64.898587488,
  "average-update-row-microsec": 16.249536657,
  "average-wait-microsec": 515.813130987,
  "average-print-microsec": 168.943209189,
  "test-duration-sec": 0.4688067220000003,
  "iteration-duration-sec": 0.0004582665904203326,
  "measured-duration-sec": 0.2902219,
  "explicit_wait_in_sec" : 0.0050000,
  "iteration-measured-duration-sec": 0.000283696871945259,
  "rules-engine-stats": {
      "slices": [
        ...
      ],
      "totals": {
          "scheduled": 3222,
          "invoked": 3222,
          "pending": 0,
          "abandoned": 0,
          "retry": 0,
          "exception": 0,
          "individual": {
            ...
          }
      },
      "calculations": {
          "avg-latency-ms": 0.17920235878336438,
          "max-latency-ms": 1.0,
          "avg-exec-ms": 0.01,
          "max-exec-ms": 0.17,
          "individual": {
            ...
          }
      }
  }
}
```

A summary of the various fields and blobs are as follows:

- `source`
  - the source suite file and line number within that file that generated this blob
- `configuration`
  - a summary of the relevant fields in the `mink.conf` file
- `iteration`
  - harvested from `output.delay`
  - number of steps that were enacted as part of the test
- `return-code`
  - return code from `return_code.json`
  - generated from return code of `test.sh`
  - see [this section](#test-return-codes) for specific decodings
- `duration-sec`
  - harvested from `duration.json`
  - amount of time that it took to execute `run.sh`
- `stop-pause-sec`
  - harvested from `output.delay`
  - amount of time that the test paused at the end of its execution
  - allows time for any information in `gaia_stats.log` to be written
- `wait-pause-sec`
  - harvested from `output.delay`
  - sum of total time used by waits performed after the `step` commands
- `explicit_wait_in_sec`
  - harvested from `output.delay`
  - sum of total time used by the `w`ait command from the `commands.txt` file
- `print-duration-sec`
  - harvested from `output.delay`
  - time spent in the `step_and_emit_state` method just to print the current state
- `test-duration-sec`
  - `duration-sec` minus `stop-pause-sec` minus `wait-pause-sec` minus `print-duration-sec`
  - approximation of the run time of the test
- `iteration-duration-sec`
  - `test-duration-sec` divided by `iteration`
  - approximation of the run time of a single step within the test
  - this field will not appear if no steps were reported in the `iteration` field
- `start-transaction-sec`
  - harvested from `output.delay`
  - amount of time spent in each step starting the transaction for the step
- `inside-transaction-sec`
  - harvested from `output.delay`
  - amount of time spent performing the workload for the step
- `end-transaction-sec`
  - harvested from `output.delay`
  - amount of time spent in each step committing the transaction for the step
- `update-row-sec`
  - harvested from `output.delay`
  - amount of time spent in each step updating database rows
- `average-start-transaction-microsec`
  - calculated by dividing the `start-transaction-sec` metric by `iteration`
- `average-inside-transaction-microsec`
  - calculated by dividing the `iteration-duration-sec` metric by `iteration`
- `average-end-transaction-microsec`
  - calculated by dividing the `end-transaction-sec` metric by `iteration`
- `update-row-sec`
  - calculated by dividing the `end-transaction-sec` metric by `iteration`
- `average-wait-microsec`
  - calculated by dividing the `wait-pause-sec` metric by `iteration`
- `average-print-microsec`
  - calculated by dividing the `print-duration-sec` metric by `iteration`
- `measured-duration-sec`
  - optional field triggered by use of the [Toggling Measurements On and Off](#toggling-measurements-on-and-off) command
  - duration that occurred between the measurement being toggled on and off
- `iteration-measured-duration-sec`
  - optional field triggered by use of the [Toggling Measurements On and Off](#toggling-measurements-on-and-off) command
  - approximation of the time of a single step within the measurement
  - this field will not appear if no steps were reported in the `iteration` field
- `rules-engine-stats.slices`
  - raw slices harvested from `gaia_stats.log`
- `totals.*`
  - aggregated values over all `rules-engine-stats.slices`
- `calculations.*`
  - weighted average/maximum values over all `rules-engine-stats.slices`

  - aggregation of the `average-start-transaction-microsec` file from `summary.json`
- `inside`
  - aggregation of the `average-inside-transaction-microsec` file from `summary.json`
- `commit`
  - aggregation of the `average-end-transaction-microsec` file from `summary.json`
- `update-row`
  - aggregation of the `average-update-row-microsec` file from `summary.json`
- `wait`
  - aggregation of the `average-wait-microsec` file from `summary.json`
- `print`
  - aggregation of the `average-print-microsec` file from `summary.json`


#### Slices and Individual Fields

For most examinations of the test data, the top-level information is all that is
required to complete the examination.  However, in certain cases, understanding
the individual data that was aggregated to provide the top-level information is
required.  In those cases, the `slices` and `individual` fields are used.

The `slices` field is present under the `rules-engine-stats` field, beside the
`totals` field and the `calculations` field.  Generated from the `gaia_stats.log` file,
the values are the standard 6 `total` fields, the standard 4 `calculations` fields, and
the `thread-load-percent` field.  These are the source fields for the aggregated values
under the `totals` and the `calculations` fields.

Under the `slices` field, the `totals` field, and the `calculations` field are the
sets of `individual` fields.  Those sets of `individual` fields are used to represent
the per-ruleset statistics that are present in the `gaia_stats.log` file.  Immediately
under the `individual` field is the identifier used to identify which rule is being
summarized beneath it.  This field is the line number in the related `*.ruleset` file,
with each measurement underneath it.

```json
"rules-engine-stats": {
    "slices": [
        {
            "thread-load-percent": 5.32,
            "scheduled": 1237,
            "invoked": 1237,
            "pending": 0,
            "abandoned": 0,
            "retry": 0,
            "exception": 0,
            "avg-latency-ms": 0.18,
            "max-latency-ms": 1.88,
            "avg-exec-ms": 0.01,
            "max-exec-ms": 0.11,
            "individual": {
                "[25]": {
                    "rule-line-number": "[25]",
                    "rule-additional-name": "incubator_ruleset::1_sen~",
                    "scheduled": 1182,
                    "invoked": 1182,
                    "pending": 0,
                    "abandoned": 0,
                    "retry": 0,
                    "exception": 0,
                    "avg-latency-ms": 0.18,
                    "max-latency-ms": 1.39,
                    "avg-exec-ms": 0.01,
                    "max-exec-ms": 0.11
                },
                ...
            }
        }
    ]
}
```

#### Group Test JSON Blob

For a group of single tests, repeated `smoke` tests for example, the output will look something
like the following.  As the intention of this example is to provide an understand of the shape
and content of the data, most of the information in the arrays has been replaced with ellipses (...).
In each case, the ellipses is replaced by 0 or more data objects.

```json
    "smoke": {
        "source": {
            "file_name": "/../tests/suite-something.txt",
            "line_number": 2
        },
        "iterations": [
            1023,
            ...
        ],
        "return-code": [
            0,
            ...
        ],
        "test-duration-sec": [
            0.46579434499999983,
            ...
        ],
        "measured-duration-sec": [
            28.814934225,
            ...
        ],
        "start-transaction-sec": [
            5.018328143,
            ...
        ],
        "inside-transaction-sec": [
            15.364705248,
            ...
        ],
        "end-transaction-sec": [
            8.407552821,
            ...
        ],
        "iteration-duration-sec": [
            0.00045532194037145634,
            ...
        ],
        "iteration-measured-duration-sec": [
            0.000268875,
            ...
        ],
        "rules-engine-stats": {
            "totals": {
                "scheduled": [
                    3225,
                    ...
                ],
                ...
            },
            "calculations": {
                "avg-latency-ms": [
                    0.1725705426356589,
                    ...
                ],
                ...
            }
        },
        "test_runs": {
            "smoke_1": {
              ...
            },
            ...
        }
    }
}
```

The big difference to the individual test reports is that the relevant aggregated information
is presented at the top of the blob as arrays.  The `iterations` field, the `return-code`,
the `test-duration-sec` field, and the `iteration-duration-sec` fields are all presented as
arrays to allow for the further interpretation of the values to be determined by the reader.
Similarly, the `rules-engine-stats` are presented as arrays of values from the individual
tests, except for the `slices` field which is omitted at this level.

Finally, the `test_runs` field contains one element for every test that was executed within
the group of tests.  This information is the contents of the `test-results` directory for
that test when it completed.  The name of each element is the name of the test, followed
by the `_` character, followed by the position of the test.  Therefore, in the above example,
the first single test as part of the `smoke` group of tests is `smoke_1`.  If there are
10 tests in that group of tests, the first test will be `smoke_1` and the last test will
be `smoke_10`.

## Modifications To The Incubator Example

As the Incubator example is a common example that everyone has experience with,
it made sense to use it as a base for the first integration test.  However, to
be used as the base application for this integration test suite, some small
modifications were required.  While the codebases are still closely related,
sharing common code, this application has been renamed to `Mink` or "Modified
INKubator" to remind test framework users of the difference.

### Command Line

#### Debug Mode

To provide for a solid way to execute tests, a new `debug` command mode was
added.  When specified, the usual `begin simulation` and `end simulation`
commands from the main menu are not available.  Instead, they are replaced
with the `step simulation` command.  Replacing the simulation's proceeding
on a 1 second timer, the `s`/`step simulation` code forces the user to take each
step individually.  In addition, the `z`/`step simulation and emit state` is a
variation of the `step simulation` command that emits a json blob with the
state of the database after the step has completed.

#### Showj Mode

The `showj` command mode is like the `show` command except that it
outputs the state of the database as a JSON blob and then exits.  While the
display is not as compact as the `show` command, the `showj` command's JSON
blob is more structured.

Whether you use the `show` command or the `showj` command boils
down to personal preference.

### Main Menu Commands

To facilitate the `debug` command mode, a collection of new main menu commands
was added.

As part of the testing framework, the usual use of the debug command mode is
for each test to provide for a `commands.txt` file that contains one line for each command
to be executed.  This file is the redirected into the `run.sh` script as part
of the test itself.  Each command in that file is simply the same command that
would be entered if the user were interacting with the application directly.

#### Toggling Measurements On and Off

The `o`/`toggle measurement on and off` command toggles the capturing of
the duration measurement on and off.  While the framework currently only supports
the capturing of a single measurement, that single measurement can be used to
great effect.  Normally, the most accurate scope that is recorded and reported
on is the `test-duration-sec` field of the JSON blobs (see above).  This is a
field calculated by taking the `duration-sec` field and subtracting the
`stop-pause-sec` field, the `wait-pause-sec` field, and the `print-duration-sec`
field.

As those are all high-level measurements, taken over the entire
debug script, they might not provide the required accuracy.  That is where
these commands can be used to provide that accuracy.  By toggling the measurement
on before the interested area in the script and off when it has completed, the
measurement is focused only on that one area of the script.  Once toggled off,
that duration will be reported in the `measured-duration-sec` field of the
`summary.json` file, along with its `iteration-measured-duration-sec` measurement.

#### Step Commands

The `s`/`step simulation` command takes most of the code in the `simulation`
method's while loop and moves it to the `simulation_step` method.  To further
support the idea of an increasing progression of time, the wrapper method
`step` increases the timestamp `g_timestamp` and then calls the `simulation_step`
method.

The `z`/`step simulation and emit state` command is like the `s`/`step simulation`
command, but some extra packaging around the calling of the `step` method.
Calling the `step_and_emit_state` method, that method then calls the `step` method
for the `s`/`step simulation` command, but follows it up with code to wait for
the engine to finish processing the changes, following that up by dumping out a
JSON representation of the database.

The introduction of the `s` command was primarily geared around being able to
proceed with the simulation at a much quicker pace than once a second.  Once
that was accomplished, the `z` command was added to get a predictable record
of the changes that occurred at each step of the simulation.

For example:

```
s
```

will take a single step.  The same example, but with a `z`, will take a single
step, but wait for the step's side effects to subside and print the current
state of the database as a JSON blob.

##### Multiple Step Command

Instead of asking a test developer to insert many step commands in
a `commands.txt` file, a shortcut is available for use.  If a positive integer
is specified directly after the `s` or `z` command, it will be interpreted as
a request to repeat that command the specified number of times.  For example:

```
z1000
```

is asking that the `z` command be executed 1000 times.

##### Future

A logical combination of the two above commands is to provide a step command
that waits for the processing of any changes before starting the next command.

#### Wait Command

Added initially to allow for some cooldown after any initialization and before any
block of heavy testing, the `w` command allows for a number to be specified
after the `w`.  This is the number of milliseconds that the application will
wait before returning for the next command.

For example:

```
w10
```

will wait for 10 milliseconds before proceeding.

As mentioned in the opening sentence, this command was initially added to
allow for some cooldown to occur between any initialization and the actual
testing started.  The intention here is to provide a buffer between the two
sets of actions, hopefully isolating them from each other.

#### Comment Command

As the normal test usage of the `debug` command mode and `tests.sh` is to
redirect the `commands.txt` file for the test into the `run.sh` script,
it was advantageous to be able to include comments.  For example, the
actual `commands.txt` file for the `smoke` test looks like this:

```
# Turn chicken portion on.
m
c
on
m
# Turn puppy portion on.
m
p
on
m
# Wait 5 milliseconds to allow the turn on effects to finish.
w5
# Take 1023 steps.
z1023
```

## Gaia Test Configuration

While there are a number of different settings in the `gaia.conf` file, the
test framework currently only focuses on two of those settings: the
`thread_pool_count` setting and the `stats_log_interval` setting.  The other
settings are set to the following
values:

```ini
[Database]
data_dir = "/var/lib/gaia/db"
instance_name="gaia_default_instance"
[Catalog]
[Rules]
log_individual_rule_stats = true
rule_retry_count = 3
```

These settings can be verified by examining the `mink.conf` file in the
`test-results` directory or the directory of a specific test within the
`suite-results` directory.

### Configuration File Locations

These two configuration settings can be specified on a per-test basis or an all-test
basis.  If the `config.json` file is in the same directory as the test's `command.txt`
file, that configuration will take precedence and is only applied to that specific
test.  If the `config.json` file is in the general `tests` directory, the configuration
will be applied to any test that does not specify its own `config.json` file.

### Thread Pool Count

The `thread_pool_count` setting specifies the number of threads in the Rule Engine
thread pool, available for processing.  This is beyond the thread used to make any
changes to the database, kicking off the firing of rules in the rule engine.

Valid values for this configuration value are `-1` (as many as possible) and any
positive integer.  To specify a single thread in the thread pool,
use the following `config.json`:

```json
{
    "thread_pool_count" : 1
}
```

The default value for this setting is `-1`.

### Gaia_Stats.log Interval

The `stats_log_interval` setting specifies the number of seconds between
the writing of individual "slices" in the `gaia_stats.log` file.  This
file is the primary source of information on what has happend in the rule
engine during the execution of a test scenario.

Valid values for this configuration value are any integer between (and including)
1 and 60.  To specify a slice duration of 5 seconds, use the following `config.json`:

```json
{
    "stats_log_interval" : 5
}
```

The default value for this setting is `2`.

## Below The Hood

### Properties Script File

The `properties.sh` script was created to keep as much of the configuration
and processing of the integration tests generic.  Going forward with more integration
tests on other applications, the expectation is that this file will grow to capture
all relevant settings.  As such, it is included into each of the remaining script files with
the following lines:

```bash
# shellcheck disable=SC1091
source "$SCRIPTPATH/properties.sh"
```

### Install Script File

The `install.sh` script file takes one argument which is the directory to install the
project to.  To ensure that the state of the project can be properly contained,
this script is used within the integration tests to install the project into the
`/tmp/test_$PROJECT_NAME` directory, which is `/tmp/test_mink` for this project.
By following this practice, any unintended dependencies are avoided, improving
the reproducibility of the tests.

### Build Script File

The `build.sh` script file is a more flexible wrapper around the CMake and make
applications.  Besides the default building of the project, the `-f`/`--force`
flag is the next most popular, deleting any existing `build` directory, forcing
the build of the project from the start.

### Lint Script File

The `lint.sh` script file can be run after the build has completed to verify the
correctness of the project code.  These verifiers fall into two categories:
formatters and analyzers.  The formatters are applied using company-standard and
industry-standard formats to ensure the source code is consistently formatted.
The analyzers use various methods to look for common issues based on industry
standards.

By virtue of their design, the formatters will always try and adjust the source
code format to follow all the guidelines without changing the meaning of the source
code.  As a contrast, the analyzers will provide errors when an analyzer rule has
been violated.  To proceed to get a clean analysis by this script, any returned
errors need to be addressed by fixing or suppression before it will continue.

In addition to executing this script on its own, the command line `./build.sh --lint`
can be used to automatically execute this script after a successful build has completed.

The currently implemented formatters and analyzers are:

| Tool Name | Category | Language |
| --- | --- | --- |
| `clang-format` | Formatter | C++ |
| `clang-tidy`  | Analyzer | C++ |
| `shellcheck` | Analyzer | Bash |
| `black` | Formatter | Python |
| `Flake8` |Analyzer | Python |
| `PyLint` | Analyzer | Python |

### Run Script File

The `run.sh` script file is a wrapper around directly calling the application
binary with the correct parameters.  While a user accessing the executable may
decide to invoke the application directly or with their own script, a test
framework must have a reliable and straightforward way to execute the application with
as few complications as possible.

To that extent, this `run.sh` script has a few options that are useful from a
testing point of view.  The `-a`/`-auto` flag allows the script to detect
whether the executable is older than any of the source files, and if so,
automatically invoke the build script (`build.sh`) referenced in the last section.  The
`-g`/`--config` flag allows for a custom configuration file to be supplied
to the application.  Finally, if the `-c`/`--csv` flag is specified, the
executable will take any JSON output provided in an `output.json` file and
try and produce an `output.csv` file from it.

### Test Script File

The `test.sh` script file is where the preparation of the other script files
starts to pay off.  This script is used to run the test scenario specified on
the command line from
start to finish, reporting on what happened during the execution of that test.
As one of the primary goals is to keep the test framework as generic as possible,
using the scripts specified in the above sections and helper Python files is
essential to maintaining that generic viewpoint.

The usual workflow for executing a test scenario looks like this:

- save the current directory
- create a new test directory
- install the project into the test directory using `install.sh`
- change the directory to the test directory
- build the project in the test directory using `build.sh`
- prepare any configuration, if specified
- clear the `test-results` directory
- execute the test scenario using `run.sh`, capturing information related to that execution
- compare the expected output with the actual output
- store any information related to the execution int the `test-results` directory
- restore the currently directory and output the success of the script

Note that if there are any errors in the workflow, the workflow jumps to
the last step and exits.

The `test.sh` script has a few options that can change that workflow.  The flag
`-ni`/`--no-init` flag is used to bypass the install and build steps outlined in the
workflow described above.  While this option should be used with caution, it can speed
up execution times for invoking a group of related tests.  Another flag is the
`-vv`/`--very-verbose`, used as a supercharger for the `-v`/`--verbose` option.
This flag will display verbose information for the `test.sh` script as well as passing
the `-v`/`--verbose` flag to each of `install.sh`, `build.sh`, and `run.sh`.  When
diagnosing issues with the execution of the test scenario, the extra information
provided can often point to the cause of the error.

In addition, the `-l`/`--list` flag specifies that a list of available test scenarios
should be output, followed by the script terminating.  This is provided as an uncomplicated
way to determine what the available tests are without having to perform a `ls` command
and comb through the output.

### Suite Script File

The `suite.sh` script file is provided to supercharge the `test.sh` script
by grouping those tests together in a readily executable package.

The usual workflow for executing a test suite looks like this:

- save the current directory
- clear the `suite-results` directory
- install and build the project once
  - create a new test directory
  - install the project into the test directory using `install.sh`
  - change the directory to the test directory
  - build the project in the test directory using `build.sh`
- for each line in the `suite-<name>.txt` file
  - before each invocation of `test.sh -ni`, pause for 15 seconds to allow for some cooldown
  - execute the test scenario according to the information on that line
    - with just the name of the test scenario, execute it once using `test.sh`
    - followed by the `repeat <n>` suffix, execute it `n` times
  - copy the contents of `test-results` into a directory under `suite-results` named for the invoked test
- summarize the results by iterating over the content of the `suite-results` directory
- restore the currently directory and output the success of the script

Note that if there are any errors in the workflow, the workflow jumps to
the last step and exits.

The `suite.sh` script has no options that change that workflow.  However, the
`s`/`--slack` flag can be specified to output relevant information about the
execution of the suite to the Slack channel `#test`.  While not that useful when
executing a test suite locally, it is useful when the script is executed as part
of a series of automated tests.  In addition, to normal status information, the
`summary.json` produced by this script is also published to Slack.

Note that as the automated test scenario is exercised more, expect other
flags to be added to control what information is sent to Slack and how.

Like the `test.sh` script, the `-l`/`--list` flag specifies that a list of available test suites
should be output, followed by the script terminating.  This is provided as an efficient
way to determine what the available tests are without having to perform a `ls` command
and comb through the output.

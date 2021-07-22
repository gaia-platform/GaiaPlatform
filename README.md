# Gaia Platform - Incubator Integration Test Suite

As much as possible, this project has been engineered to be copied to
another directory and used for another integration tests, with a minimal
set of changes required.

## TL;DR

If all you want to do is run the tests, and have blind faith in them,
please check out the sections on
[Executing Single Tests](#executing-single-tests) and
[Executing Test Suites](#executing-test-suites)

## Introduction

This is a modified version of the
[Incubator example](https://github.com/gaia-platform/GaiaPlatform/tree/master/production/examples/incubator).
It has been altered to provide for integration test functionality.

The goal of this project is to take a simple example and turn it into
an integration test suite that is predictable, maintainable, portable,
and observable.  Given that statement, if asked why we decided to take
a given route on solving an issue, the explanation will always
focus on one of those four goals.

To that extent, a lot of the changes and commands described in the
following sections are there to provide closer alignment with those goals.
The substantial change is that the "1 second timer" part of the Incubator example
has been amended to include a debug mode.  This debug mode allows for
the frequency between updates to be controlled by the test, instead of
waiting for another second to pass.

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

High level tests are driven by specifying two files in a unique directory under `tests`:

- `commands.txt`
  - input to push through the various levels of menus in the incubator application
- `expected_output.json`
  - output that is expected from the incubator application

For more information on these files, refer to the following section on
[Creating a Test](#creating-a-test).

In addition, the `config.json` file can be specified at the `tests` level
or the individual test level.  A `config.json` file local to a test will
always supersede the `tests` level `config.json`.  For more information
on this file, refer to the section on
[Gaia Test Configuration](#gaia-test-configuration)

#### Creating a Test

Each test is assigned the unique name of the directory containing the
`commands.txt` and the `expected_output.json` files.

The `commands.txt` file is a simple file with a single command on each line.
Those commands, exception for
[a few added commands](#main-menu-commands),
are the same as the commands for the standard Incubator example. For example,
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

If you place this file where you can see it and engage the incubator in
normal mode in a separate window, you can type each one of these commands
on a line, hit enter, and follow what each one is doing.  The first group
of commands switches to the incubators menu (`m`), selects chickens (`c`),
enabled the chicken incubator (`on`), and then returns to the main menu (`m`).
The second group of commands performs the same action, but on the puppy (`p`)
incubator instead.
Once that setup is completed, the `w5` waits for 5 milliseconds, and then
invokes the `step` function `1023` times before exiting the simulation.

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
  - simple file created by `diff`ing the test's `expected_output.json` file with the generated `output.json` file
- `gaia_stats.log`
  - clean version of the log file generated by the rule engine
- `incubator.conf`
  - configuration file generated from the `config.json` file
- `output.delay`
  - various test measurements, mostly various time-based delays, in JSON format
- `output.json`
  - output file generated by executing the commands

## Test Return Codes

A given test can have one of a small set of return codes.  A return code
of `0` means that the tests passed with no errors.  For the other return
codes:

- `1` - some part of the setup of the suite itself did not complete properly
- `2` - a failure occurred during the setup or the teardown of the test itself
- `5` - the underlying invocation of the `run.sh` script failed
- `6` - the test completed, but the data in the `expected-output.json` file did not match the actual output

## Executing Test Suites

Executing a test suite is accomplished by executing the `./suite.sh` script.
If executed without any additional commands, the `smoke` suite will be executed.
A full set of available suites can be found by executing
`./suite.sh -l`, and a specific suite can be executed by executing
`./suite.sh <suite-name>`.

As these test suites are intended to be executed as part of a CI/CD pipeline,
broadcasting the information to a monitoring channel is advantageous.  To start,
that channel is the `#test` channel on the Gaia Team's Slack.  Publishing the
ongoing information about a suite's execution can be enabled by adding
the `-s`/`--slack` flag to the command line used to execute the suite.

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

#### Repeating A Test

In specific cases, there is a need to run a given test multiple times within a suite.
When that is required, the following format is required:

```
smoke repeat 5
```

That line with execute the `smoke` test 5 times as part of the current suite.

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
15 second pause before each test.  Currently, this is not configurable.

### Summary.json File

The elements in the `summary.json` file are organized by the name of the test that
they are executing.  Regardless if it is a single test or a group of tests, that first
level element is always the name of that test.  After that, there is some divergence
on how the information is stored to more efficiently aggregate the information from a
group of tests.

#### Single Test JSON Blob

For a single test, for example the `smoke` test, the output will look something
like the following example.  Note that the information in the `slices` blob has been
omitted as most of that information is aggregated in other places within the
overall JSON blob.

```json
"smoke": {
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
  "test-duration-sec": 0.4688067220000003,
  "iteration-duration-sec": 0.0004582665904203326,
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
          "exception": 0
      },
      "calculations": {
          "avg-latency-ms": 0.17920235878336438,
          "max-latency-ms": 1.0,
          "avg-exec-ms": 0.01,
          "max-exec-ms": 0.17
      }
  }
}
```

A summary of the various fields and blobs are as follows:

- `configuration`
  - a summary of the relevant fields in the `incubator.conf` file
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
- `rules-engine-stats.slices`
  - raw slices harvested from `gaia_stats.log`
- `totals.*`
  - aggregated values over all `rules-engine-stats.slices`
- `calculations.*`
  - weighted average/maximum values over all `rules-engine-stats.slices`

#### Group Test JSON Blob

For a group of single tests, repeated `smoke` tests for example, the output will look something
like the following.  As the intention of this example is to provide an understand of the shape
and content of the data, most of the information in the arrays has been replaced with ellipses (...).
In each case, the ellipses is replaced by 0 or more data objects.

```json
    "smoke": {
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
        "iteration-duration-sec": [
            0.00045532194037145634,
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
modifications were required.

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
would be entered if the user were interacting with the incubator application
directly.

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
test framework currently only focuses on one of those settings: the
`thread_pool_count` setting.  The other settings are set to the following
values:

```ini
[Database]
data_dir = "/var/lib/gaia/db"
instance_name="gaia_default_instance"
[Catalog]
[Rules]
stats_log_interval = 2
log_individual_rule_stats = true
rule_retry_count = 3
```

These settings can be verified by examining the `incubator.conf` file in the
`test-results` directory or the directory of a specific test within the
`suite-results` directory.

### Thread Pool Count

The `thread_pool_count` setting is the only thing that is currently configurable
on a per-test basis or an all-test basis.  This setting specifies the number of threads
in the Rule Engine thread pool, available for processing.  This is beyond the
thread used to make any changes to the database, kicking off the firing of rules in the
rule engine.

Valid values for this configuration value are `-1` (as many as possible) and any
positive integer.  For example, to specify a single thread in the thread pool,
use the following `config.json`:

```json
{
    "thread_pool_count" : 1
}
```

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
`/tmp/test_$PROJECT_NAME` directory, which is `/tmp/test_incubator` for this project.
By following this practice, any unintended dependencies are avoided, improving
the reproducibility of the tests.

### Build Script File

The `build.sh` script file is a more flexible wrapper around the CMake and make
applications.  Besides the default building of the project, the `-f`/`--force`
flag is the next most popular, deleting any existing `build` directory, forcing
the build of the project from the start.

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

# Template

# Template Integration Test

This Template workload provides for a decent-level of infrastucture required to get a new workload up and running with a "standard" Gaia example.

The strength of this Template workload over the Barebones workload is its local usability.  These scripts are meant to work together locally to provide for a quicker turnaround time when diagnosing and fixing issues.  For example, when starting off a new workload and trying to get my model compiling and running, my favorite command line is `./run.sh -a debug ./tests/smoke/commands.txt` to automatically build (if needed) and execute the application in debug mode.

## See Also

For a workload that contains only the minimally required infrastructure, please consult the [Barebones Workload](https://github.com/gaia-platform/GaiaPlatform/tree/main/production/tests/workloads/barebones).

For a workload that has everything setup with a very simple model and some simple measurements, please consult the [XX Workload](#template).

## Workload Input

A large part of executing the workload is already provided for using the `install.sh`, `build.sh` and `test.sh` files. Unlike the previously mentioned [Barebones Workload](https://github.com/gaia-platform/GaiaPlatform/tree/main/production/tests/workloads/bareboness), the following scripts are provided for use as-is and should not be modified:

- `build.sh`
- `install.sh`
- `test.sh`
- `run.sh`
- `lint.sh`

Instead, the `properties.sh` file is provided as a common reference file that is used by each of those scripts.  The use of the `properties.sh` file allows for any required changes to scripts to occur in one place.  This allows for those scripts to be updated and copied across workloads without worrying about overwriting changes.  In addition to using the `properties.sh` for executing the workload, the `summarize_test_results.py` Python script is used to accurately coalesce multiple possible files into a single `test-summary.json` file.  In a similar manner, the `summarize_test_results.py` module allows for the collection of data from discrete sources in a workload independant manner.

Please consult the `--help` option for the above scripts for more information on the local invocation options that they provide.

### Test Directories

To allow for multiple profiles and test types, the `tests` directory is used.  Each profile or "mode" is referred to by the name of the directory.  For example, the `suite-template.txt` file in the framework has the following content:

```text
#
# Simple test suite for the template tests.
#

workload template

smoke
smoke repeat 2
perf
perf repeat 2
```

That input assumes that the `template` workload will specify a `smoke` mode and a `perf` mode.  To satisfy that request, the current `template` workload must have a `tests/smoke` directory and a `tests/perf` directory.

#### Contents

There are four files that are usually found in each test directory, two are mandatory and two are optional.

The required `test.properties` file, as documented in the [Barebones workload](../barebones/readme.md#test-properties-file), describes the test being executed. Also required, the `commands.txt` file provides for a file that is provided by standard input to the application.  This is allows for a single application to alter its behavior based on input that it defines.  This also keeps the specification of "what should my test do" as generic as possible.  It works, because it is up to the application to determine what the contents passed through to standard input mean.

Not required, but highly recommended, is an optional `readme.md` file, providing:

- a short one sentence description of the test
- the purpose for having this test
- general discusion about the test, why it was done this way, why it is important, etc.

This file does not have to be anything amazing, just enough information that it gives a reader a good idea of what the workload is about.

The optional `expected_output.json` is used with integration tests to verify that the output from the application is as expected.  The comparison is kicked off if the execution of the `run.sh` script comes back with a successful return code, there is an `expected_output.json` file in the test directory, and there is a `$BUILD_DIRECTORY/output.json` file. If those conditions are met, a standard `diff` is used to compare the expected output to the actual output, depositing any results from that `diff` command into the file `expected.diff`.  If the `diff` command detected a difference, the return code for the test is set to `6` to indicate that the test was successful but did not output its expected output.

### Properties File

There are four main sections to the template properties file.

The first section contains variables that are specific to the names used by this project.  Assuming that every file will be named after the workload, a new workload will probably need to replace `template` with the name of that workload in each instance that occurs in this section.

```sh
# ----------------------------------------------------
# Variables that are specific to this project.
# ----------------------------------------------------
```

The second section contains variables that are used by more than one script and are kept in this common location.
The variables in this section should not be touched.

```sh
# ----------------------------------------------------
# Variables that are general to this group of scripts.
# ----------------------------------------------------
```

#### Test Entry Points

The third section provides for an entry point into the test script. This section provides for the `copy_extra_test_files` entry point and starts with the following:

```sh
# -----------------------------------------------------------------
# Functions to specify project specific information for test.sh.
#
# This is placed in this file to make the core scripts as agnostic
# as possible for reuse.
# -----------------------------------------------------------------
copy_extra_test_files() {
    echo ""
}
```

There are cases where additional files need to be copied to the `TEST_RESULTS_DIRECTORY` after tests have completed.  A good example of this is additional debug files that were generated somewhere else but pivotal to debugging issues.

#### Execution Entry Points

The fourth section starts with the following:

```sh
# -----------------------------------------------------------------
# Functions to specify how to process the command line with run.sh.
#
# This is placed in this file to make the core scripts as agnostic
# as possible for reuse.
# -----------------------------------------------------------------
```

##### Setting The Command Name

In this section, the first line is:

```sh
export TEST_COMMAND_NAME="debug"
```

The `TEST_COMMAND_NAME` is passed by the `test.sh` script to the `run.sh` script to specify a debug command.  This is provided so that the built application can have multiple modes, with the `TEST_COMMAND_NAME` named command specifying a mode suitable for automated tests.

##### Display Commands in Usage

The next part is the `show_usage_commands` function which can be used to show more information about available commands:

```sh
show_usage_commands() {
    echo ""
    echo "  $TEST_COMMAND_NAME               Run the simulation in debug mode."
}
```

While the `run.sh` script is used by the `test.sh` script to execute the application, the `run.sh` can also be used locally to verify the application before using it in a test.  Those commands should be documented here so that they can be displayed to any users of the `run.sh` script.

##### Executing the Application

The final part is the `execute_commands` function.  While the provided default implementation provides for a simple handling of the command, the only goal here is to get the application executing.  In the example, the `execute_commands` function assigns the two parameters to local variables before determining if the command is acceptable.  If it is the `debug` command, the `process_debug` function is called.  In the example, the `process_debug` does not call out to any application, but has commented code to show how easily this can be done.

It is within this function that any generation of results outside of the appplication should be performed.  One of the parameters that is passed to the `execute_commands` is there to indicate that the user would like a CSV file to be generated if possible.  This is just one example of extra processing that can be added on a workload-by-workload basis.

## Workload Output

### Test Results Directory

The contract for the contents of the `test-results` directory for this workload are the same as they are for the [Barebones workload](../barebones/readme.md#workload-output).  The main difference is that the contents of this workload do a certain amount of the heavy lifting to produce the required end results.

To start with, the `test.properties` and `workload.properties` files are automatically copied into the `test-results` directory.  The `test.properties` file is copied from the `tests` directory and the `workload.properties` is copied from the root workload directory. No extra effort is required.

### Test Summary File

The `test-summary.json` is a JSON file that contains measurements and is expected as part of the contract.  The `summarize_test_results.py` script is invoked to collect any data from files generated by the application and any third party applications, coalescing those discrete sources into a singular `test-summary.json`.  An example of this file as created by the Template workload is as follows:

```json
{
    "iterations": 1,
    "sample-ms": 1,
    "configuration-file": "/tmp/test_suite/template.conf",
    "return-code": 0,
    "duration-sec": 0.072406131
}
```

These values were collected from various source, as follows:

- `interations` and `sample-ms`
  - hardcoded into `summarize_test_results.py`
- `configuration-file`
  - configuration used for the application
- `return-code` and `duration-sec`
  - created by `test.sh` and written out as `return-code.json` and `duration.json`

Note that summaries of the two Gaia logs are already handled by the main suite scripts, and do not require any parsing by the `test-summary.json` script.

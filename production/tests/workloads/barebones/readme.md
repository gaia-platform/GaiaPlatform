# Barebones Integration Test

This integration test workload provides for the minimal infrastucture needed to get a new integration test workload up and running.  Please note that this project is VERY basic, and does little in the way of capturing failure scenarios or handling weird cases.

## See Also

For a workload that sets a lot of the requirements and infrastructure up, but not the XXX, please consult the [Template Workload](#template).

For a workload that has everything setup with a very simple model and some simple measurements, please consult the [XX Workload](#template).

## Workload Input

To start executing the workload, three files are required: `install.sh`, `build.sh` and `test.sh`.  Note that for the purposes of the Barebones workload, these files are specifically focused to be executed by the Integration Suite test framework.  The similar scripts in other workloads are larger as they also support running those scripts locally, and different options for how to run those scripts locally.

The following sections describe these script files and their usage.

### Install.sh

`./install.sh <install-directory>`

The `install.sh` script is invoked from the main suite script with a single parameter: `install-directory`, the directory to install to.  To help keep the test isolated, the suite provides that directory as a location where the test can be built and executed without disrupting anything in the workload directory.

While it is recommended that the entire workload directory structure is copied over to this directory, the only requirement is that any files that are needed to build the project or test the project are copied over.

#### Sample Behavior

For this Barebones workload sample, the script copies everything in the workload directory into the `install-directory` and then changes the directory to the `install-directory`.

### Build.sh

`./build.sh --verbose`

The `build.sh` script is invoked from the main suite script to build any artifacts needed for the test workload.  Note that when this script is invoked from the suite, the current directory has already been set to the install directory passed to the `install.sh` script.

#### Sample Behavior

For this Barebones workload sample, the script simply echoes text to the output.

### Test.sh

`./test.sh --very-verbose --no-init --num-threads <num-threads> --directory <install-directory> <test-name>`

The `test.sh` script is invoked from the main suite script once for each test to be executed.  As far as this script is concerned, there are no repeat tests, only the one test that it is being asked to execute and report on.

Some of the parameters are provided for local execution help, such as `--very-verbose` for very verbose mode and `--no-init` no initialize.  The `--directory` parameter is followed by the name of the install directory passed to the `install.sh` script.

At the end of the parameters/flags, the `test-name` argument specifies the name of the test to be executed.  It is completely up to the script

If the number of threads is specified by the suite, the `--num-threads` parameter will be used to pass that value to the script. It is the responsibility of the `test.sh` script to do any work necessary to honor that setting.

The return code

- 0 - AOK
- 1 - test script failed itself for some reason
- 4 - specified test to execute was not available for any reason
- 5 - internal error executing any inner scripts/programs from within test.sh
- 6 - for integration tests, any expected results did not match actual results.

#### Sample Behavior

For this Barebones workload sample, the script provides for a constant test result.

After setting some variables, the `parse_command_line` and `save_current_directory` functions are called to set things up for the test.  The `create_results_directory` function is then called to create the required results directory as documented in [Test Results Directory](#test-results-directory) below.

The values passed in to the command line are then echoed for illustration purposes, followed by a simple comparison to set the `test.properties` file in the output, as [documented below](#test-properties-file).

To provide the actual measurements to help performance tests, a `test-summary.json` file is created with the following sample values:

```json
{
    "return-code" : 0,
    "sample-ms" : 1
}
```

This sample [summary file](#test-summary-file) provides for one count-based measure, the `return-code` field, and for one duration-based measure, the `sample-ms` field.  The `return-code` field is required, and the suite will fail if that is not provided.  If a custom Gaia configuration file it to be used for the tests, the path to that file should be specified in the optional `configuration-file`.  If provided, the suite will provide a breakdown of that file in the summary itself, giving more context to the results.

With the "test" completed, the `workload.properties` file is copied into the test results directory.  At this point in the script, any other files, such as the `gaia.log` and `gaia_stats.log` files should be copied into that directory along with any other relevant test files.  By using this mechanism, those extra files may provide insight into the test behavior to help diagnose failures.

## Workload Output

### Test Results Directory

The test results directory is the primary method for conveying information about the test results to the integration suite.  While other files may be present in the directory for debugging purposes, the integration suite required the following files to always be present: `test.properties`, `workload.properties` and `test-summary.json`.

### Test Properties File

The `test.properties` file currently has a single line conveying a single property:

```ini
test-type=integration
```

or

```ini
test-type=performance
```

This file is used to inform the suite of what type of test was executed. The main purpose of this file is to affect the focus of how to interpret the results of the executed test.  For example, for integration tests, the focus is on the amount of success versus the amount of failure.  For performance tests, that ratio is only part of the information, the other part being statistics on how a series of those tests executed.

### Test Summary File

The `test-summary.json` is a JSON file that contains measurements

#### Test Summary Measures

The following test summary measures are recognized:

- seconds
 - specify a suffix of `-sec` or `-seconds`
- milliseconds
 - specify a suffix of `-ms` or `-millisec`
- microseconds
 - specify a suffix of `-microsec`
- nanoseconds
 - specify a suffix of `-nanosec`
- count
 - any other suffix

If the measure is being specified in the following section on [Performance Legend](#performance-legend), then simply omit the dash character (`-`) at the start of the prefix.

### Workload Properties File

The `workload.properties` file is used to help the Integration Suite display the information from one or more related `test-summary.json` files in a meaningful manner.

### Aggregations

The first section, `Aggregations` is used to specify if a field from the `per-test` section should be aggregated over a series of tests and how.  The format within the file is very simple:

```ini
[Aggregations]
return-code=if-present
```

The name of the element to use is the key, and the value is one of the following three values:

- `always` - always aggregate the value, failing if it is not present in the `per-test` section
- `if-present` - only aggregate the value if it is present in the `per-test` section
- `if-present-or-none` - aggregate the value if it is present in the `per-test` section, otherwise aggregate `None`

The slight difference in functionality depends on the usage and consistency of the data.  If the workload is created so that the specified field will ALWAYS be there, even under error conditions, then the `always` value is safe.  The `if-present` value can be used if there is a slight variation in the results that can cause a low percentage of the tests to fail.  This value will allow that test to fail without aborting the suite.  This is useful in situations where a longer running test is being executed multiple times.  Finally, the `if-present-or-none` value is mostly for external consumption, providing an indication that there should have been a value to aggregate, but it was missing.

#### Performance Legend

The second section `PerformanceLegend` is used to specify what performance metrics will be displayed with the full range of statistics calculations executed against a set of results.

```ini
per-test.iteration-measured-duration-sec=iter-total:microsec
per-test.average-start-transaction-microsec=start
.separator=
rules-engine-stats.calculations.avg-latency-ms=avg-latency:microsec
```

This example was taken from the Mink workload.  The first line specifies that the displayed performance information should contain two different sections, the two sections being separated by the `.separator=` line.

When executed with that specific example above, an example output of the `smoke-time-only` test is:

```
---
Test: smoke-time-only
---
  Source        /suite-definitions/suite-smoke.txt:9
  Test Type     performance
  Test Results  Success

    Measure           Value
    ------------  ---------
    iter-total    311.857µs
    start          45.224µs

    Measure           Value
    -----------  ----------
    avg-latency   490.000µs
```

The key for each line specified the path to use to arrive at the performance information to be displayed.  So the key `per-test.iteration-measured-duration-sec` specifies that the `iteration-measured-duration-sec` field of the `per-test` section will be used for the data.  According to [Test Summary Measures](#test-summary-measures), because its name ends with `sec`, the measurement will be assigned a unit of `seconds` by default.

The value for each line specifies the display name to associate with the key in additional to optional conversion information.  For the first line, the display name is `iter-total` and the conversion information is `microsec`. This will show the title `iter-total` at the start of any display line with the measure on it, and the measure displayed will be in microseconds.  Note that the second line does not have a converstion specified, therefore it will use its default unit of microseconds.

Note that in the fields specified in this section must be aggregated, so any `per-test` fields that need displaying must have entries in the `[Aggregations]` section of the file.

### Lint.sh

While its usage is not required, a sample `lint.sh` file has been provided to provide for good workload hygene.  It currently has the Python and C++ portions disabled, as this workload only contains Bash scripts.

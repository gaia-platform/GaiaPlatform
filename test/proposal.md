# Directory Reset Proposal

Based on feedback provided by Dax, I have thought very
heavily about what a good directory structure is that
keeps concerns clearly defined and separated.

Here is what I came up with.

## What This Solves

> 1. [future/suggestion] clearly separate out the test harness from the specific scenario.  I can't tell if this is done already but "incubator" is only one workload.  But I see you have test and then test/tests, but the incubator files are under test.  The moment you add another scenario, this structure won't scale.
> 3. [future]We should think about the test directory structure more carefully.  I.e. test/stress, test/perf/ test/integration, etc... in other words a single directory is not scalable at all. (related to 1)

This reorganization provides a solution that I believe addresses these
issues.  The file/directory reorg directly addresses the first concern by
using the workload as the main organizational unit under the base directory.
That is to say if we come up with a `HelloWorld` workload, we would create
a directory to contain that workload alongside the `Mink` directory.

As to the second concern, I believe that any organization of the files
and directories under a workload's `tests` directory is under the auspices
of that workload.  If you have a workload that has 3 tests total, it might
not make sense to split them up.  More than 10 tests, perhaps it is time
to split them up.

That being said, in retooling the project to support the workload model,
I believe it is prudent to ensure that the `suite.sh` script can support
both directory structures.

## Base Directory

Proposed contents for the base directory are:

```text
python (dir)
suites (dir)
lint.sh?
suite.sh
README.md
reset_database.sh
restart_server.sh
reporting_template.md
Pipfile?
```

- suite.sh would require small changes to specify the workload to use for a given test in a suite
  - would depend on `workload/tests` and `workload/test.sh` to run tests and adhere to interface

## Workload Directories

Proposed contents for the base directory are:

```text
python (dir)
tests (dir)
build.sh
CMakeLists.txt
install.sh
lint.sh
{project files: .cpp, .ddl, .ruleset}
Pipfile?
properties.sh
run.sh
test.sh
```

- each workload has its own directory
- first one is `mink` for the `Modified INKubator` workload
- this directory should be self-contained (shared Python files?), with known external interface points

### Possible?

- may need a `workload-test.sh` to query a workload for available tests
  - other option would be to open a terminal, cd to the workload dir, and `./test.sh --list`

## Test Files Directory

The known external interface points for these directories are:

- local `test.sh` file
- local `build.sh` file
- generated `test-results` directory

To be clear with this intent, the `suite.sh` must expect that the workload
directories contain two scripts, `build.sh` and `test.sh`, that allow the
workload project to be built and to be executed.  Once a specified test
has executed, the results of that test must be present in the `test-results`
directory, allowing them to be imported in the suite's results folder.

In creating the test files directory for each test, I believe there is
some extra benefit that we can arrive at by clearly identifying each
test directory as having given properties.  Once of these properties would
be the type of test that the directory contains.

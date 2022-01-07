# GitHub Actions

This subproject exists to generate the various YAML files used by GitHub Actions and [stored here](https://github.com/gaia-platform/gaia-platform/blob/master/.github/workflows/README.md).

## Before Proceeding

You probably want to read some of the notes on [GitHub Actions](https://docs.github.com/en/actions) before proceeding.
For the most part, our jobs are easy to read, but it will give you some foundation to use when looking at the generated files.


## Nomenclature

When we refer to the *GDev.cfg tree*, we are referring to the collection of `gdev.cfg` files in the `GaiaPlatform` project.
These configuration files were put in place to faciliate the building of a docker image that provides a developer environment container.
For more information, please go to the [GDev subproject](https://github.com/gaia-platform/gaia-platform/blob/master/dev_tools/gdev/README.md).

## The Configuration Files

### Options and Modifying Configuration Information

There are a small set of conditionals that can respond to options provided as parameter to the scripts, as detailed in following sections.
These conditionals allow for one definition of truth on how to build the project, with multiple viewpoints.
Each conditional must be placed at the start of the line that it is to take effect for.
A small note that whitespace is not considered for conditional placement.
Indenting by four characters is the default when using whitespace to denote a continued command line.

The first conditionl is the single `{enable_if('option')}` conditional.
This conditional simply checks to see if that single option is present and evaluates to true if so.
The second conditional is the negation of that, the `{enable_if_not('option')}` conditional, evaluating to true if the option is not present.
The third conditional is the multiple version of the `enable_if` conditional, the `enable_if_any` conditional.
The `enable_if_any` conditional accepts between 1 and 3 options to trigger on, each option surround with apostrophes and separated from other options by a single comma character.
To balance this, the `enable_if_none` is the final conditional.
This conditional is constructed like the `enable_if_any` conditional, but only triggers if none of the options are met.

### Location Matters

When using conditionals, the location of a conditional matters.

In this example:

```
{enable_if('CI_GitHub')}mkdir -p /build/production
```

the conditional will apply only to the line in question.

That makes this set of lines interesting:

```
{enable_if_not('GaiaLLVMTests')}LSAN_OPTIONS=detect_leaks=0 \
    {enable_if('CI_GitHub')}VERBOSE=1 \
    make -j$(nproc)
```

In this example, there are two conditionals that seem to contradict each other.
Because of the line continuation character at the end of the first line, the `GaiaLLVMTests` option applies to the first and third line, while the `CI_GitHub` option only applies to the middle line.

In addition, a conditional can be applied to an entire section:

```
{enable_if('GaiaRelease')}[package]
produces:/build/production/gaia-${{ env.GAIA_VERSION }}-${{ github.run_id }}_amd64.deb
cd /build/production
```

If this is done, the entire section is considered to be encapsulated in that conditional.

## The Scripts

To make composing the information from the GDev.cfg tree easier, a series of scripts are used.
While the top-level script, `compose_github_actions.sh` should be the only one that you need for most purposes, knowledge of the other scripts is very useful for debugging.

### `munge_gdev_files.py`

This script is used to aggregate and filter information from the GDev tree for use in other scripts.
In its base form, this script is called with:

```
munge_gdev_files.py --directory ../../production
```

to process the entire production GDev tree.

#### `Raw` Parameter

Once the entire GDev tree has been loaded into memory, the `--raw` option can be used to provide a JSON representation of the entire tree.
This option is very useful to make sure that the raw information is being parsed from the individual `gdev.cfg` files properly.
When presented, note that each set of lines are presented as line pairs.
The first entry for the line pair is the actual line as it was parsed from the `gdev.cfg` file.
The second entry contains the dominant conditional to be applied to that line.

For example, this line from the `[run]`:

```
{enable_if('CI_GitHub')}mkdir -p /build/production
```

is translated into one of the lines in the `[run]` section:

```
[
      [
        "mkdir -p /build/production",
        "enable_if('CI_GitHub')"
      ]
]
```

By loading the information in the Gdev files in this manner, we are able to properly deal with the options in a separate step, keeping the loading simple.

#### `Option` Parameter

To specify each option, the `--option` parameter is used.
The options are stored in the `__available_options` variable and are currently:

- `GaiaRelease`
- `ubuntu:20.04`
- `CI_GitHub`

With the exception of the last option, those options are all standard GDev options used to alter behavior.
The final option is used to specify that a given behavior is specific to the requirements for this family of scripts.
A good example of this is in the GDev files for the Git-based subprojects.
While the dockerized use of that data requires the use of `rm -rf *` to removed extra artifacts, that action is harmful to the use of the subprojects in the creation of YAML files for GitHub Actions.
Therefore, a lot of those subprojects will have the line:

```
rm -rf *
```

changed to:

```
{enable_if_not('CI_GitHub')}rm -rf *
```

to allow that line to function properly in both environments.

#### `Section` Parameter

To deal with the options in the final step, the `--section` parameter is used.
This parameter instructs the script to output data properly formatted for the section in question.
Those sections are stored in the `__valid_section_names` variable and are currently:

- `gaia`
- `apt`
- `pip`
- `git`
- `web`
- `env`
- `copy`
- `artifacts`
- `package`
- `tests`
- `installs`
- `pre_run`
- `run`
- `pre_lint`
- `lint`

While stored with square brackets around them (i.e. `[gaia]` instead of `gaia`), they must be specified on the command line without square brackets.
This is mainly done to avoid confusion in scripts where square brackets are used to alter the flow of scripts.

Note that the output for each section is formatted for use with the sibling scripts `build_main_yaml.py`.
While some output is generated without any headers, some rendered information contains multiple build steps and must be formatted properly to ensure the multiple build steps are properly rendered.
[JDW: Need something more here on that, just not sure what.]

### `build_job_header.py`

This script is used to put together the header for the entire `main.yml` file.
It does not contain any difficult logic as there is no need for any on a cross-job level.
If that stipulation changes, this script will need to change to accomodate those requirements.

### `build_job_section.py`

While the `munge_gdev_files.py` script is used to get information about a specific part of the job, it is the responsibility of this script to piece the information together for a single job with the `main.yml` workflow.

### `compose_github_actions.sh`

The `build_job_section.py` script is used to compose a job, and this script is used to group those jobs together to form the master `main.yml` file.
The `compose_github_actions.sh` script has too main sections: predefined jobs and installed jobs.

#### Predefined Jobs

The predefined jobs section is for those jobs that are statically in the build process.
Currently, these are:

- Lint
- Core (requires Lint)
- SDK (requires Core)
- Debug_Core (requires Core)
- LLVM_Tests (requires Core)

The use of the word `requires` refers to the GitHub Actions requirement, not any other requirement.
Based on this ordering, the Lint job starts first.
Once it completes successfully, the CORE job starts.
Once it completes successfully, the other three jobs start.

#### Installed Jobs

The second type of jobs, installed jobs, are specified in the root `gdev.cfg` file.
Currently set to:

```
[install:Integration]
:Suite Logs=${{ github.workspace }}/production/tests/suites
cd ${{ github.workspace }}/production/tests
./smoke_suites.sh

[install:Samples]
:Test Logs=${{ github.workspace }}/dev_tools/sdk/test/test.log
cd ${{ github.workspace }}/dev_tools/sdk/test
./build_sdk_samples.sh > test.log

[installs]
Integration
Samples
```

this specifies the jobs that will be created that do not depend on any of the builds, on the packaged build artifact.
The `[installs]` section denotes the names of the jobs that will be created.
This was purposefully done in this manner, as opposed to scanning for the individual sections, so that install sections can be added and removed from the build while retaining their configuration.

For each section under `[installs]`, there is a corresponding section that starts with `[install:`, ends with `]` and has the name of the install between them.
While optional, any line of the specification that begins with a `:` denotes a file to be made into an artifact for that job.
In the above example:

```
:Suite Logs=${{ github.workspace }}/production/tests/suites
```

specifies that an artifact named `Suite Logs` will be uploaded from the contents of the path following the `=` sign.
Any other line is explicitly written to the output for the job.
Keep in mind that any script lines added in this fashion can assume that the package generated in the `SDK` section has been installed and is ready to use.

### `verify_actions.sh`

So, because of our history and current use of gdev and gdev.cfg trees, we are caught in a Catch-22.
To properly put everything together, if any of those gdev.cfg files changes, we want to build a new `main.yml` file.
But because GitHub Actions requires the `main.yml` file to be present in the commit itself, there is a question about how to properly tackle updating the `main.yml` file.

Until those scenarios are addressed, and even after as a preventative measure, the `verify_actions.sh` script is present to safeguard against out-of-date `main.yml` files.
This script builds a *new* YAML file, but redirects it to another file path.
It then compares the two files, the one it just generated against the checked in `main.yml` file for any difference.
If there are any differences, it prints out what they are, gives a nice reminder to run the `compose_github_actions.sh` script, and fails.
Otherwise, it succeeds.

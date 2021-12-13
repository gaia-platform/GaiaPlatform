# GitHub Actions

This subproject exists to generate the various YAML files used by GitHub Actions and [stored here](https://github.com/gaia-platform/gha/blob/master/.github/workflows/README.md).

## Before Proceeding

You probably want to read some of the notes on [GitHub Actions](https://docs.github.com/en/actions) before proceeding.
For the most part, the jobs are easy to read, but it will give you some foundation to use when looking at the generated files.


## Nomenclature

When we refer to the *GDev.cfg tree*, we are referring to the collection of `gdev.cfg` files in the `GaiaPlatform` project.
These configuration files were put in place to faciliate the building of a docker image that provides a developer environment container.
For more information, please go to the [GDev subproject](https://github.com/gaia-platform/gha/blob/master/dev_tools/gdev/README.md).

## The Configuration Files

### Options and Modifying Configuration Information

There are a small set of conditionals that can respond to options provided as parameter to the scripts, as detailed in following sections.
These conditionals allow for one definition of truth on how to build the project, with multiple viewpoints.
Each conditional must be placed at the start of the line that it is to take effect for.

The first conditionl is the single `{enable_if('option')}` conditional.
This conditional simply checks to see if that single option is present and evaluates to true if so.
The second conditional is the negation of that, the `{enable_if_not('option')}` conditional, evaluating to true if the option is not present.
The final conditional is the multiple version of the `enable_if` conditional, the `enable_if_any` conditional.
The `enable_if_any` conditional accepts between 1 and 3 options to trigger on, each option surround with apostrophes and separated from other options by a single comma character.

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

xxx

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
- `env`
- `web`
- `pre_run`
- `run`
- `copy`
- `artifacts`
- `package`
- `tests`

While stored with square brackets around them (i.e. `[gaia]` instead of `gaia`), they must be specified on the command line without square brackets.
This is mainly done to avoid confusion in scripts where square brackets are used to alter the flow of scripts.

Note that the output for each section is formatted for use with the sibling script `build_main_yaml.py`.
While some output is generated without any headers, some rendered information contains multiple build steps and must be formatted properly to ensure the multiple build steps are properly rendered.
[JDW: Need something more here on that, just not sure what.]

### `build_main_yaml.py`

While the `munge_gdev_files.py` script is used to understand the parts of the GDev configuration files, it is the responsibility of this script to fit the information together when asked.


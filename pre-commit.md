# Producing Clean Code

As part of our move to GitHub Actions, we added various checks and balances to our CI environment.
This effort is partially to ensure that we follow every coding guideline and do so in a way that is non-obtrusive.

## Pre Commit Checks

The checks in the following section are executed using the [Pre-Commit framework](https://pre-commit.com/) and the `.pre-commit-config.yaml` file.
This framework can easily be installed on developer machines to verify any changes pass these checks before commit.
After installation using `pip install pre-commit`, executing `pre-commit run --all-files` from the root of the repository will check every file in the repository.

Note that `pre-commit` is also implemented as part of our GitHub Actions workflow, and will fail the workflow if they do not pass.

This file is mainly to include notes on the various pre-commit checks we use, as well as ones that we have tried and discarded.
For more information on why we are using any of these hooks, please refer to the [Coding Guidelines](docs/coding-guidelines.md) document.

### Generic Checks

- check for merge conflicts
  - make sure we are not checking in any partially merged changes
- check for YAML formatting
  - YAML is used for GitHub Actions, and small mistakes are common
- check for TAB character in files
  - none of our projects should use TAB characters
- check for CRLF sequence in files
  - since we are all developing on Ubuntu, none of our files should have the Windows end of line sequence
- single empty line at end of file
  - posix compliance
- no trailing spaces at end of line
  - hard to see in some editors

### C++ Files

- check with `clang-format`
  - ensure that every c++ file adheres to our formatting standards
- check with `clang-tidy`
  - enables the `ENABLE_CLANG_TIDY` flag for CMake during the GitHub Action builds
  - due to include file requirements, this is part of the builds
- (ON DECK) check with `oclint`
  - have heard good things about this from online forums

### Bash Script Files

Scripts are being used for test automation and CI building.

- check with `shellcheck`
  - verify that scripts all follow best practices
- shell files are executable
  - each script should be marked as executable

### Python Files

- check with `Black`
  - ensure that every Python file adheres to the standard Python formatting
- check with `Flake8` and `PyLint`
  - ensure that scripts all follow best practices

## Tried and Rejected Scans

- `cppcheck`
  - team did not feel there was enough benefit for the false positives that were being reported
- `cpplint` (includes `include-what-you-use`)
  - look for well known issues and prevent them
  - waiting on https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1848 and https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1849

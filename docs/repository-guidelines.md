# Repository Guidelines

## Table Of Contents

- [File Guidelines](#file-guidelines)
  - [Using Pre-Commit](#using-pre-commit)
  - [General File Guidelines](#general-file-guidelines)
  - [C++ File Guidelines](#c++-file-guidelines)
  - [Python File Guidelines](#python-file-guidelines)
  - [Bash File Guidelines](#bash-file-guidelines)
  - [Markdown File Guidelines](#markdown-file-guidelines)
- [Code Submission Guidelines](#code-submission-guidelines)
  - [GitHub Workflows and Submissions](#gitHub-workflows-and-submissions)

## File Guidelines

These guidelines apply to any files that are committed to this project.
Where possible, these guidelines use our [Pre-Commit](../pre-commit.md) infrastructure to verify that the guidelines are being applied uniformly against our portions of the code base.
To that extent, the following directories are not scanned for adherence to guidelines:

- `data`
- `scratch`
- `third_party`

Please note that these guidelines are generalizations, and are not absolutes.
If there is an exception to the guidelines, they are on a case by case basis and should be documented as such.
However, where possible we should be trying to adhere to these guidelines unless there is a show-stopper that is preventing us from doing so.

### Using Pre-Commit

Part of the standard Git application is the ability to specify actions to take before a commit is submitted to the repository.
While people often "roll their own", a popular option is to use the [Pre-Commit](https://pre-commit.com) application.
This application provides a very clean and modular way to add actions (referred to as checks) to a Git repository.

#### Fixers and Checkers

There are two types of Pre-Commit hooks that are available.
The Pre-Commit homepage does not go into that concept until later in the (very long) home page, which has caused confusion.
The Checker hooks scan for a given set of circumstances and fail the check if that set of circumstances is present.
If you run Pre-Commit again without addressing those circumstances, you should expect to see the same failure.

The Fixer hooks scan for a given set of circumstances and fail the check if that set of circumstances is present.
However, unlike the Checker hooks, these hooks will attempt (and usually succeed) to fix the issue that they failed on.
As such, if you execute Pre-Commit again, you should expect to see the failure disappear and have changed files appear when you enter `git status` in the repository.
This process is specific to Pre-Commit's design.
It was done this way to allow you a chance to verify any changed files before committing them to the repository.

#### Installing Pre-Commit

The instructions at the [Pre-Commit Homepage](https://pre-commit.com) are very easy to follow for installation of the pre-commit hooks.
Once the Pre-Commit application has been installed, you will need to follow the instructions for installing it as a pre-commit hook by using the command line: `pre-commit install`.
In addition, as we use `shellcheck` for validating Bash scripts, you will need to do `sudo apt-get install -y shellcheck` to allow the shellcheck hook to work properly.

#### Invoking Pre-Commit

Generally speaking, after following the above instructions, the Pre-Commit application will be executed as part of your local Git commit process.
However, if you want to execute the tool by itself before committing, you can invoke it manually using `pre-commit` with no options.
If all check pass, you should see a series of lines that end in passed or skipped:

```text
clang-format.............................................................Passed
Test shell scripts with shellcheck...................(no files to check)Skipped
Non-executable shell script filename ends in .sh.....(no files to check)Skipped
```

Given this output, the observation is that the repository is in a state where there is at least one change to a C++ file and no changes to any Bash files.

If there are any errors, you should see output similar to:

```text
Trim Trailing Whitespace.................................................Failed
- hook id: trailing-whitespace
- exit code: 1
- files were modified by this hook

Fixing docs/coding-guidelines.md
```

Given this output, the observation is that the file `docs/coding-guidelines.md` (this file) contained at least one line that had a trailing whitespace.
Furthermore, because the failure report started with `Fixing`, it can be inferred that the `trailing-whitespace` hook is a fixer hook, and fixed the issue in that same file.

### General File Guidelines

#### Lines Should Not Have Trailing Whitespace

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | trailing-whitespace |

Not all editors display trailing whitespace clearly.
As such, it can often be difficult to spot the trailing whitespace, which may influence the data on that line in an unexpected manner.
As none of the languages we use requires whitespaces at the end of a line, our guideline is to not have it present.

#### Every File Should End With A Single Blank Line

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | end-of-file-fixer |

It is a [common standard](https://stackoverflow.com/questions/2287967/why-is-it-recommended-to-have-empty-line-in-the-end-of-a-source-file#:~:text=The%20empty%20line%20in%20the,can%20handle%20the%20EOF%20marker.) to end each source file with a single blank line.
We adhere to this guideline, as in a Linux world, is just makes sense.
In addition, GitHub also flags the lack of the terminating endline too when it displays the files, so why do things that result in red icons in GitHub Pull Requests?

#### Files Should Not Contain Multiple Blank Lines Unless Required

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| local | fix_double_empty_lines |

With very few exceptions, such as Python and [PEP8](https://www.python.org/dev/peps/pep-0008/), multiple blank lines do not impose any additional meaning to a file.
To keep things consistent, the guideline is to remove multiple blank lines in favor of one blank line unless there is a strict requirement to keep the multiple lines.
Outside of Python file, one example is the [licenses file](../production/licenses/LICENSE.third-party.txt) file, where preserving the file in its original format is important.

#### Files With Merge Conflicts Should Not Be Checked In

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | check-merge-conflict |

This is hopefully a bit of an obvious guideline.
However good our editors are, sometimes a file with merge conlict markers can become part of a commit and make it into the code base.
Therefore, the guideline to not have merge conflict markers in our codebase is checked for as part of our pre-commit checks.

#### YAML Files Should Be Cleanly Parseable

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | check-yaml |

This is hopefully another obvious guideline.
As we try and automated more with our codebase, the number of YAML files present is increasing.
While the format for a YAML file is pretty easy to understand, it is also pretty easy to get something slightly wrong.
Therefore, we check that only cleanly parseable YAML files are added to our codebase.

#### Copyright Notices Are Required

To ensure that we are following legal guidelines regarding company intellectual property, all source code must be properly annotated with one of the following preface in the encompassed sections.
As script files may also need to start with the [shebang](https://en.wikipedia.org/wiki/Shebang_(Unix)) sequence, the various forms of those prefix text blocks are outlined below.

##### Python Executable

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | python_executable_license_check |

```Python
#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
```

This is the prefix that all Python files are checked for, except in the one case that follows.

##### Python Non-Executable

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | python_non_executable_license_check |

```Python
#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

```

The sole exception to the earlier prefix is for [this file](../production/tools/tests/gaiat/lit.cfg.py).
That file is used to provide configuration for the `lit` testing framework, and as such, is not executable.

##### Bash Executable

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | bash_license_check |

```Python
#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

```

This is the prefix that all Bash files are checked against.

##### CMake Build Files

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | cmake_license_check |

```python
#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

```

This is the prefix that all `*.cmake` and `CMakeLists.txt` files are checked against.

##### CMake Build Files

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | db_license_check |

```sql
---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

```

This is the prefix that all `*.ddl` and `*.sql` files are checked against.

##### CMake Build Files

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/pre-commit-hooks | db_license_check |

```cpp
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

```

This is the prefix that all `*.cpp`, `*.hpp`, and `*.inc` files are checked against.

### C++ File Guidelines

#### Consistent File Formatting Using Clang-Format

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/pre-commit/mirrors-clang-format | clang-format |

To have consistent formatting in our C++ files, the `clang-format` tool is applied using [this settings file](../.clang-format).
Our coding style is roughly equivalent to the LLVM coding style with a couple of small changes.

Note that in the past, various actions have resulted in a broken build.
The first action is `clang-format` reordering the includes in a file, breaking the build in the process.
To avoid that side-effect, please read [this article](https://stackoverflow.com/questions/37927553/can-clang-format-break-my-code).
The second action is changing versions of the Clang-Format tool.
When that has happened, we have had to update our [settings file](../.clang-format).
While the second action happens less frequently, it has happened enough times that we though it prudent to mention it here.

Note that copies of third-party files (like `json.hpp`) and committed generated files (like `production/catalog/src/gaia_catalog.cpp`) are eligible for being excluded from being scanned.

#### Linting C++ Using Clang-Tidy

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| None (in build) | clang-tidy |

While most of us are diligent in writing correct code, there are classes of issues that we can easily miss.
To prevent us from hitting those issues, the `clang-tidy` tool is integrated into the build scripts, and is applied using [this settings file](../.clang-tidy).
At the moment, any produced warnings will only show up in the compiler output, but will not fail a build.
However, in the near future, we hope to enable a setting in `clang-tidy` that would change the warnings to errors and fail the build.
As that is our goal, please do your best to reduce the number of warnings and to not introduce any new warnings.

### Python File Guidelines

#### Using Standard Python Tools

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/psf/black | black |
| https://github.com/PyCQA/flake8 | flake8 |
| https://github.com/PyCQA/pylint | pylint |

The "holy trinity" of development tools for most Python developers are Black, Flake8, and PyLint.
Black is simply a code formatter that most Python developers use in its default settings, which are PEP8 compliance.
Both Flake8 and PyLint are common linters that provide enough benefit that most Python developers keep them around.

### Bash File Guidelines

#### Bash Scripts Should Be Clearly Identifyable As Such

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/jumanjihouse/pre-commit-hooks | script-must-have-extension |

We have lots of different types of files in our code base, so there is a clear advantage to having the filename provide a clear indication of what type of file it is.
This guideline merely states that any Bash script should end with a `.sh` extension, and the `script-must-have-extension` hook enforces this.

#### Linting Bash Using Shellcheck

| Pre-Commit Hook | Hook Id |
| -- |  -- |
| https://github.com/jumanjihouse/pre-commit-hooks | shellcheck |

Shellcheck is a very efficient linter for Bash shell scripts.
It is primed with a good set of checks that aim to avoid the issues that are most often found with Bash scripts.

### Markdown File Guidelines

* Keep sentences on a single line.
  This makes it easier to update them.
* Use empty lines for separation of titles, paragraphs, examples, etc.
  They are ignored when rendering the files, but make them easier to read when editing them.
* Use `back-quoting` to emphasize tool names, path names, environment variable names and values, etc.
  Basically, anything that is closer to coding should be emphasized this way.
* Use **bold** or *italics* for other situations that require emphasis.
  Bold can be used when introducing new concepts, like **Quantum Build**.
  Italics could be used when quoting titles of documents, such as *The Art of Programming*.
  These situations should be rarer.
* Use links to reference other project files like the [production README](production/README.md), for example.

## Code Submission Guidelines

Once any changes have been verified locally, passing local tests and verifying that the changes comply to the [File Guidelines](#file-guidelines), the next step is to submit the changes to the Git repository.

[Ed. Note: Some guidelines about how to format your commit messages and your PR messages TBD.  Probably a link into Confluence.]

### GitHub Workflows and Submissions

The repository's GitHub workflows can be leveraged for these three main usage patterns:

1. before a *Pull Request* is created, using a manual workflow trigger on a non-`master` branch
1. as part of a *Pull Request* against the `master` branch
1. as part of merging a *Pull Request* against the `master` branch

#### Before a Pull Request

Some developers have expressed a need to externally verify that their changes work properly and do not have unintended side effects.
While our configuration does not automatically kick off GitHub workflows when changes are pushed to non-`master` branches of the main repository, they can be started using the process described on [this GitHub webpage](https://docs.github.com/en/actions/managing-workflow-runs/manually-running-a-workflow).
When executed in this fashion, the workflow will execute all jobs contained within the workflow.
While this may be overkill in some cases, the developer may cancel the workflow at any time during its execution.
In this way, developers can execute as many or as few of the extra checks as they require for their particular situation.

#### Part of Pull Request

It is good to assure the reviewer that (at least) basic steps have been undertaken to verify that the code that they are reviewing is correct.
By default, the GitHub workflow will run a shortened set of jobs against the source branch of a *Pull Request*.
In addition, this workflow is executed within the scope and context of that branch, keeping any changes within the branch until the *Pull Request* is approved.
As a reviewer, it is at your discretion as to whether you approve a Pull Request or not before all jobs in the shortened workflow have completed.

#### Part of Merging a Pull Request

Once the merging conditions for the repository are met, the `Squash and Merge` button will be enabled and the branch that is under review will be merged.
That merge action triggers the execution of the full workflow against the `master` branch with the new changes applied to it.

#### Looking at the GitHub Workflow in Action

To look at any of the currently running workflow actions, please use the [Actions Tab](https://github.com/gaia-platform/GaiaPlatform/actions) on the main repository page.
The standard colors and symbols apply to the workflows: a red X for a failed workflow, a green checkmark for a successful workflow, and a yellow dot with an "orbitting circle" for a workflow that is still in progress.
The GitHub pages are very good at updating when they need to update, but they still occasionally require a manual browser page refresh to set things straight.
In addition, whenever a workflow has a failed job or a cancelled workflow, an email is sent to the owner of the branch.

#### For More Technical Information...

If you want to know more about how our GitHub Actions workflow is put together, please check out our [GitHub Actions README](../.github/workflows/README.md).

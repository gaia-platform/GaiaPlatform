#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import sys
import time
import subprocess

__available_options = ["GaiaRelease", "ubuntu:20.04", "ubuntu:18.04", "CI_GitHub", "Debug", "GaiaLLVMTests"]

__file_prefix = """name: Main

on:
  push:
    branches:
      - master
  pull_request:
    branches:
      - master
      - jack/gha

jobs:
  build:
    runs-on: ubuntu-20.04
    env:"""

__section_1 = """    steps:
      - name: Checkout Repository
        uses: actions/checkout@master

      - name: Setup Python 3.8
        uses: actions/setup-python@v1
        with:
          python-version: 3.8

      - name: Install PipEnv
        uses: dschep/install-pipenv-action@v1
        with:
          version: 2021.5.29

      - name: Install Required Applications
        run: |
          sudo apt-get update && sudo apt-get install -y """

__section_2 = """
      - name: Install Required Python Packages
        run: |
          python3.8 -m pip install --user atools argcomplete"""

__section_3 = """
      - name: Install Required Third Party Git Repositories
        run: |"""

__section_4 = """
      - name: Install Required Third Party Web Packages
        run: |"""

__section_5 = """
      - name: Pre-Build {section}
        run: |"""

__section_6 = """
      - name: Build {section}
        run: |"""

__section_7 = """
      - name: Tests
        run: |"""

__section_end = """
      - name: Done
        run: |
          echo "Done"
"""

__copy_section = """
      - name: Create /source and /build directories.
        run: |"""

def __execute_script(command_list):
    """
    Execute a remote program, returning when it is done.
    """
    with subprocess.Popen(
        command_list,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as process:

        return_code = None
        while return_code is None:
            time.sleep(0.2)
            return_code = process.poll()

        out_lines = process.stdout.readlines()
        error_lines = process.stderr.readlines()
        return return_code, out_lines, error_lines

def __valid_options(argument):
    """
    Function to help argparse limit the valid options that are supported.
    """
    if argument in __available_options:
        return argument
    raise ValueError(f"Value '{argument}' is not a valid option.")

def __process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Perform actions based on a graph of GDEV configuration files."
    )
    parser.add_argument(
        "--directory",
        dest="configuration_directory",
        action="store",
        required=True,
        help="Directory to root the configuration search in.",
    )
    parser.add_argument(
        "--option",
        dest="options",
        action="append",
        help="Specify an option to use.",
        type=__valid_options,
        choices=__available_options,
    )
    return parser.parse_args()

def __create_build_map_and_ordered_list(prerun_outp):
    current_section = None
    build_map = {}
    ordered_build_list = []
    for i in prerun_outp:
        if i.startswith("cd $GAIA_REPO/"):
            current_section = i
            ordered_build_list.append(i)
            build_map[current_section] = []
        if current_section and i.strip():
            build_map[current_section].append(i)
    return build_map, ordered_build_list

def __collect_lines_for_section(section_name, section_long_name, cmd_options, show_debug_output=False):

    cmds = ["./munge_gdev_files.py", "--section", section_name]
    cmds.extend(cmd_options)
    code, env_outp, errp = __execute_script(cmds)
    if show_debug_output:
        print(f"output for {section_long_name}: {env_outp}")
    assert code == 0, f"Error getting generated {section_long_name}({code}): {errp}"
    return env_outp

def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    cmd_options = ["--directory", args.configuration_directory]
    for i in args.options:
        cmd_options.append("--option")
        cmd_options.append(i)

    # Run the munge script to create each part of the file.
    env_outp = __collect_lines_for_section("env", "environment", cmd_options)
    apt_outp = __collect_lines_for_section("apt", "apt", cmd_options)
    pip_outp = __collect_lines_for_section("pip", "pip", cmd_options)
    git_outp = __collect_lines_for_section("git", "git", cmd_options)
    web_outp = __collect_lines_for_section("web", "web", cmd_options)
    copy_outp = __collect_lines_for_section("copy", "copy", cmd_options)
    artifacts_outp = __collect_lines_for_section("artifacts", "artifacts", cmd_options)
    tests_outp = __collect_lines_for_section("tests", "tests", cmd_options)
    package_outp = __collect_lines_for_section("package", "package", cmd_options)
    prerun_outp = __collect_lines_for_section("pre_run", "pre_run", cmd_options)
    run_outp = __collect_lines_for_section("run", "run", cmd_options)

    # A small amount of adjustments to the input.
    assert len(apt_outp) == 1
    apt_outp = apt_outp[0].strip()
    assert len(pip_outp) == 1
    pip_outp = pip_outp[0].strip()
    prerun_build_map, _ = __create_build_map_and_ordered_list(prerun_outp)
    run_build_map, run_ordered_build_list = __create_build_map_and_ordered_list(run_outp)

    # Up to the jobs.build.env section
    print("---")
    print(__file_prefix)
    for i in env_outp:
        print("      " + i, end="")

    # Start of steps to `Install Required Applications`
    print(__section_1, end="")
    print(apt_outp)

    # `Install Required Python Packages`
    print(__section_2, end="")
    print(pip_outp)

    # `Install Required Third Party Git Repositories`
    print(__section_3)
    for i in git_outp:
        print("          " + i, end="")

    # `Install Required Third Party Web Packages`
    print(__section_4)
    for i in web_outp:
        print("          " + i, end="")

    # Each of the subproject sections, up to `Build production``
    for next_section_cd in run_ordered_build_list:

        if run_ordered_build_list[-1] == next_section_cd:
            print(__copy_section)
            for i in copy_outp:
                print("          " + i, end="")

        proper_section_name = next_section_cd[len("cd $GAIA_REPO/"):].strip()
        if next_section_cd in prerun_build_map:
            print(__section_5.replace("{section}", proper_section_name))
            for i in prerun_build_map[next_section_cd]:
                print("          " + i, end="")
        print(__section_6.replace("{section}", proper_section_name))
        for i in run_build_map[next_section_cd]:
            print("          " + i, end="")

    # `Tests`
    print(__section_7)
    for i in tests_outp:
        print(i, end="")

    # `Generate Package`
    for i in package_outp:
        print(i, end="")

    # `Upload *`
    for i in artifacts_outp:
        print(i, end="")

    # Done
    print(__section_end)

sys.exit(process_script_action())

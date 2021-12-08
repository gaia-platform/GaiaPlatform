#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import sys
import time
import subprocess

__available_options = ["GaiaRelease", "ubuntu:20.04"]

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
      - name: Upload CMake Logs
        if: always()
        uses: actions/upload-artifact@v2
        with:
          name: CMake Logs
          path: |
            /build/production/CMakeFiles/CMakeOutput.log
            /build/production/CMakeFiles/CMakeError.log

      - name: Tests
        run: |
          echo "cd /build/production/db/core || exit 1" > install.sh
          echo "make install || exit 1" >> install.sh
          echo "cd /build/production || exit 1" > install.sh
          echo "ctest || exit 1" >> install.sh
          chmod +x install.sh
          sudo ./install.sh
      - name: Upload LastTest.log
        if: always()
        uses: actions/upload-artifact@v2
        with:
          name: CTest Logs
          path: |
            /build/production/Testing/Temporary/LastTest.log

      - name: Generate Package
        run: |
          cd /build/production
          sudo bash -c "make preinstall"
          sudo bash -c "cpack -G DEB"
          ls -la /build/production/gaia-0.3.2_amd64.deb

      - name: Upload Package
        uses: actions/upload-artifact@v2
        with:
          name: Debian Install Package
          path: /build/production/gaia-0.3.2_amd64.deb

      - name: Done
        run: |
          echo "Done"
"""

__copy_section = """
      - name: Create /source and /build directories.
        run: |"""

def __execute_script(command_list):
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

def foobar(prerun_outp):
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

def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    cmd_options = ["--directory", args.configuration_directory]
    for i in args.options:
        cmd_options.append("--option")
        cmd_options.append(i)

    cmds = ["./munge_gdev_files.py", "--section", "env"]
    cmds.extend(cmd_options)
    code, env_outp, errp = __execute_script(cmds)
    #print("outp: " + str(env_outp))
    assert code == 0, f"Error getting generated environment({code}): {errp}"

    cmds = ["./munge_gdev_files.py", "--section", "apt"]
    cmds.extend(cmd_options)
    code, apt_outp, errp = __execute_script(cmds)
    # print("outp: " + str(apt_outp))
    assert code == 0, f"Error getting generated apt({code}): {errp}"
    assert len(apt_outp) == 1
    apt_outp = apt_outp[0].strip()

    cmds = ["./munge_gdev_files.py", "--section", "pip"]
    cmds.extend(cmd_options)
    code, pip_outp, errp = __execute_script(cmds)
    # print("outp: " + str(pip_outp))
    assert code == 0, f"Error getting generated pip({code}): {errp}"
    assert len(pip_outp) == 1
    pip_outp = pip_outp[0].strip()

    cmds = ["./munge_gdev_files.py", "--section", "git"]
    cmds.extend(cmd_options)
    code, git_outp, errp = __execute_script(cmds)
    # print("outp: " + str(git_outp))
    assert code == 0, f"Error getting generated git({code}): {errp}"

    cmds = ["./munge_gdev_files.py", "--section", "web"]
    cmds.extend(cmd_options)
    code, web_outp, errp = __execute_script(cmds)
    # print("outp: " + str(git_outp))
    assert code == 0, f"Error getting generated web({code}): {errp}"

    cmds = ["./munge_gdev_files.py", "--section", "copy"]
    cmds.extend(cmd_options)
    code, copy_outp, errp = __execute_script(cmds)
    # print("outp: " + str(copy_outp))
    assert code == 0, f"Error getting generated copy({code}): {errp}"

    cmds = ["./munge_gdev_files.py", "--section", "pre_run"]
    cmds.extend(cmd_options)
    code, prerun_outp, errp = __execute_script(cmds)
    # print("outp: " + str(prerun_outp))
    assert code == 0, f"Error getting generated prerun_outp({code}): {errp}"

    prerun_build_map, _ = foobar(prerun_outp)
    # print(str(prerun_build_map))
    # assert False

    cmds = ["./munge_gdev_files.py", "--section", "run"]
    cmds.extend(cmd_options)
    code, run_outp, errp = __execute_script(cmds)
    # print("outp: " + str(run_outp))
    assert code == 0, f"Error getting generated run_outp({code}): {errp}"

    run_build_map, run_ordered_build_list = foobar(run_outp)
    # print("outp: " + str(run_ordered_build_list))
    #print("outp: " + str(build_map))

    print("---")
    print(__file_prefix)
    for i in env_outp:
        print("      " + i, end="")
    print(__section_1, end="")
    print(apt_outp)
    print(__section_2, end="")
    print(pip_outp)
    print(__section_3)
    for i in git_outp:
        print("          " + i, end="")
    print(__section_4)
    for i in web_outp:
        print("          " + i, end="")

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

    print(__section_7)

sys.exit(process_script_action())

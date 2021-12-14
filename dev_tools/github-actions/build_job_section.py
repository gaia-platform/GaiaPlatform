#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to take the components of the various GDev.cfg files and put them
together to form the main.yaml for our build.
"""

import argparse
import sys
import time
import subprocess

__available_options = [
    "GaiaRelease",
    "ubuntu:20.04",
    "ubuntu:18.04",
    "CI_GitHub",
    "Debug",
    "GaiaLLVMTests",
]

__JOB_PREFIX = """
  {name}:
    runs-on: ubuntu-20.04
    needs: {needs}
    env:"""

__STEPS_PREFIX_AND_APT_SECTION_HEADER = """    steps:
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

__PIP_SECTION_HEADER = """
      - name: Install Required Python Packages
        run: |
          python3.8 -m pip install --user atools argcomplete"""

__GIT_SECTION_HEADER = """
      - name: Install Required Third Party Git Repositories
        run: |"""

__WEB_SECTION_HEADER = """
      - name: Install Required Third Party Web Packages
        run: |"""

__PREBUILD_SECTION_HEADER_TEMPLATE = """
      - name: Pre-Build {section}
        run: |"""

__BUILD_SECTION_HEADER_TEMPLATE = """
      - name: Build {section}
        run: |"""

__TESTS_SECTION_HEADER = """
      - name: Unit Tests
        run: |"""

__SECTION_SUFFIX = """
      - name: Done
        run: |
          echo "Done"
"""

__COPY_SECTION_HEADER = """
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
        "--job-name",
        dest="job_name",
        action="store",
        required=True,
        help="Name to give the job being created.",
    )
    parser.add_argument(
        "--option",
        dest="options",
        action="append",
        help="Specify an option to use.",
        type=__valid_options,
        choices=__available_options,
    )
    parser.add_argument(
        "--requires",
        dest="required_jobs",
        action="append",
        help="Specify the name of another job that this job depends on.",
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


def __collect_lines_for_section(
    section_name, section_long_name, cmd_options, show_debug_output=False
):

    cmds = ["./munge_gdev_files.py", "--section", section_name]
    cmds.extend(cmd_options)
    code, env_outp, errp = __execute_script(cmds)
    if show_debug_output:
        print(f"output for {section_long_name}: {env_outp}")
    assert code == 0, f"Error getting generated {section_long_name}({code}): {errp}"
    return env_outp


def __print_formatted_lines(source_output_line_list, indent=""):
    for i in source_output_line_list:
        print(indent + i, end="")


__ENVIRONMENT_SECTION = "env"
__APT_SECTION = "apt"
__PIP_SECTION = "pip"
__GIT_SECTION = "git"
__WEB_SECTION = "web"
__COPY_SECTION = "copy"
__ARTIFACTS_SECTION = "artifacts"
__TESTS_SECTION = "tests"
__PACKAGE_SECTION = "package"


def __calculate_section_lines(cmd_options):
    section_line_map = {}

    section_line_map[__ENVIRONMENT_SECTION] = __collect_lines_for_section(
        __ENVIRONMENT_SECTION, "environment", cmd_options
    )
    section_line_map[__APT_SECTION] = __collect_lines_for_section(
        __APT_SECTION, "apt", cmd_options
    )
    section_line_map[__PIP_SECTION] = __collect_lines_for_section(
        __PIP_SECTION, "pip", cmd_options
    )
    section_line_map[__GIT_SECTION] = __collect_lines_for_section(
        __GIT_SECTION, "git", cmd_options
    )
    section_line_map[__WEB_SECTION] = __collect_lines_for_section(
        __WEB_SECTION, "web", cmd_options
    )
    section_line_map[__COPY_SECTION] = __collect_lines_for_section(
        __COPY_SECTION, "copy", cmd_options
    )
    section_line_map[__ARTIFACTS_SECTION] = __collect_lines_for_section(
        __ARTIFACTS_SECTION, "artifacts", cmd_options
    )
    section_line_map[__TESTS_SECTION] = __collect_lines_for_section(
        __TESTS_SECTION, "tests", cmd_options
    )
    section_line_map[__PACKAGE_SECTION] = __collect_lines_for_section(
        __PACKAGE_SECTION, "package", cmd_options
    )

    return section_line_map


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    cmd_options = ["--directory", args.configuration_directory]
    for i in args.options:
        cmd_options.append("--option")
        cmd_options.append(i)

    section_line_map = __calculate_section_lines(cmd_options)

    # Run the munge script to create each part of the file.
    prerun_outp = __collect_lines_for_section("pre_run", "pre_run", cmd_options)
    run_outp = __collect_lines_for_section("run", "run", cmd_options)

    # A small amount of adjustments to the input.
    assert len(section_line_map[__APT_SECTION]) == 1
    apt_outp = section_line_map[__APT_SECTION][0].strip()

    assert len(section_line_map[__PIP_SECTION]) == 1
    pip_outp = section_line_map[__PIP_SECTION][0].strip()

    prerun_build_map, _ = __create_build_map_and_ordered_list(prerun_outp)
    run_build_map, run_ordered_build_list = __create_build_map_and_ordered_list(
        run_outp
    )

    # Up to the jobs.build.env section
    print(__JOB_PREFIX.replace("{name}", args.job_name))
    required_jobs
    __print_formatted_lines(section_line_map[__ENVIRONMENT_SECTION], indent="      ")

    # Start of steps to `Install Required Applications`
    print(__STEPS_PREFIX_AND_APT_SECTION_HEADER, end="")
    print(apt_outp)

    # `Install Required Python Packages`
    print(__PIP_SECTION_HEADER, end="")
    print(pip_outp)

    # `Install Required Third Party Git Repositories`
    print(__GIT_SECTION_HEADER)
    __print_formatted_lines(section_line_map[__GIT_SECTION], indent="          ")

    # `Install Required Third Party Web Packages`
    print(__WEB_SECTION_HEADER)
    __print_formatted_lines(section_line_map[__WEB_SECTION], indent="          ")

    # Each of the subproject sections, up to `Build production``
    for next_section_cd in run_ordered_build_list:

        if run_ordered_build_list[-1] == next_section_cd:
            print(__COPY_SECTION_HEADER)
            __print_formatted_lines(
                section_line_map[__COPY_SECTION], indent="          "
            )

        proper_section_name = next_section_cd[len("cd $GAIA_REPO/") :].strip()
        if next_section_cd in prerun_build_map:
            print(
                __PREBUILD_SECTION_HEADER_TEMPLATE.replace(
                    "{section}", proper_section_name
                )
            )
            __print_formatted_lines(
                prerun_build_map[next_section_cd], indent="          "
            )
        print(__BUILD_SECTION_HEADER_TEMPLATE.replace("{section}", proper_section_name))
        __print_formatted_lines(run_build_map[next_section_cd], indent="          ")

    # `Tests`
    print(__TESTS_SECTION_HEADER)
    __print_formatted_lines(section_line_map[__TESTS_SECTION])

    # `Generate Package`
    __print_formatted_lines(section_line_map[__PACKAGE_SECTION])

    # `Upload *`
    __print_formatted_lines(section_line_map[__ARTIFACTS_SECTION])

    # Done
    print(__SECTION_SUFFIX)


sys.exit(process_script_action())

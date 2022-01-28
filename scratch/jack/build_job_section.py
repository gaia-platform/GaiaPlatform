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
    "GaiaSDK",
    "ubuntu:20.04",
    "CI_GitHub",
    "Lint",
    "ubuntu:18.04",
    "Debug",
    "GaiaLLVMTests",
]

__ENVIRONMENT_SECTION = "env"
__APT_SECTION = "apt"
__PIP_SECTION = "pip"
__GIT_SECTION = "git"
__WEB_SECTION = "web"
__COPY_SECTION = "copy"
__ARTIFACTS_SECTION = "artifacts"
__TESTS_SECTION = "tests"
__PACKAGE_SECTION = "package"


__RUN_ACTION = "run"
__LINT_ACTION = "lint"
__INSTALL_ACTION_PREFIX = "install:"


__JOB_PREFIX = """
  {name}:
    runs-on: ubuntu-20.04
    {needs}{env}"""

__SLACK_SUFFIX = """
      - name: Report Job Status To Slack
        uses: 8398a7/action-slack@v3
        if: always()
        with:
          status: ${{ job.status }}
          fields: repo,message,commit,author,action,eventName,ref,workflow,job,took,pullRequest # selectable (default: repo,message)
        env:
          SLACK_WEBHOOK_URL: ${{ secrets.SLACK_WEBHOOK_URL }}
"""

# More on the Slack action here: https://github.com/marketplace/actions/action-slack
__STEPS_PREFIX_AND_APT_SECTION_HEADER = """    steps:
      - name: Checkout Repository
        uses: actions/checkout@master

      - name: Setup Python 3.8
        uses: actions/setup-python@v2
        with:
          python-version: 3.8

      - name: Install Required Applications
        run: |
          sudo apt-get update && sudo apt-get install -y """

__PIP_SECTION_HEADER_TEMPLATE = """
      - name: Install Required Python Packages
        run: |
          python3.8 -m pip install {pip}
          python3 -m pip install {pip}"""

__GIT_SECTION_HEADER = """
      - name: Install Required Third Party Git Repositories
        run: |"""

__WEB_SECTION_HEADER = """
      - name: Install Required Third Party Web Packages
        run: |"""

__PREBUILD_SECTION_HEADER_TEMPLATE = """
      - name: Pre-{action} {section}
        run: |"""

__BUILD_SECTION_HEADER_TEMPLATE = """
      - name: {action} {section}
        run: |"""

__TESTS_SECTION_HEADER = """
      - name: Unit Tests
        run: |"""

__COPY_SECTION_HEADER = """
      - name: Create /source and /build directories.
        run: |"""

__LINT_SECTION_SUFFIX = """
      - name: Set Cache Key for Pre-Commit Checks
        run: echo "PY=$(python -VV | sha256sum | cut -d' ' -f1)" >> $GITHUB_ENV

      - name: Apply Pre-Commit Checks Cache
        uses: actions/cache@v1
        with:
          path: ~/.cache/pre-commit
          key: pre-commit|${{ env.PY }}|${{ hashFiles('.pre-commit-config.yaml') }}

      - name: Execute Pre-Commit Checks
        uses: pre-commit/action@v2.0.3
        with:
          extra_args: --all-files"""

__INSTALL_SECTION_HEADER = """    steps:

      - name: Checkout Repository
        uses: actions/checkout@master

      - name: Setup Python 3.8
        uses: actions/setup-python@v2
        with:
          python-version: 3.8

      - name: Install PipEnv
        uses: dschep/install-pipenv-action@v1
        with:
          version: 2021.5.29

      - name: Download Debian Install Package
        uses: actions/download-artifact@v2
        with:
          name: SDK Debian Install Package
          path: ${{ github.workspace }}/production/tests/packages

      - name: Tests
        working-directory: ${{ github.workspace }}
        run: |
          cd ${{ github.workspace }}/production/tests/packages
          sudo apt --assume-yes install "$(find . -name gaia*)"
          cd ${{ github.workspace }}/production/tests
          sudo ./reset_database.sh --verbose --stop --database
"""

__INSTALL_SECTION_UPLOAD_TEMPLATE = """
      - name: Upload Logs
        if: always()
        uses: actions/upload-artifact@v2
        with:
          name: {name}
          path: |
            {path}"""


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
    parser.add_argument(
        "--action",
        dest="action",
        default=__RUN_ACTION,
        action="store",
        help="action to undertake",
    )
    return parser.parse_args()


def __create_build_map_and_ordered_list(prerun_outp):

    if not prerun_outp:
        return None, None

    current_section = None
    build_map = {}
    ordered_build_list = []
    for next_line in prerun_outp:
        if next_line.startswith("cd $GAIA_REPO/"):
            current_section = next_line
            ordered_build_list.append(next_line)
            build_map[current_section] = []
        if current_section and next_line.strip():
            build_map[current_section].append(next_line)
    return build_map, ordered_build_list


def __collect_lines_for_section(
    section_name, section_long_name, cmd_options, job_name, show_debug_output=False
):

    cmds = ["./munge_gdev_files.py", "--section", section_name, "--job-name", job_name]
    cmds.extend(cmd_options)
    raw_command = " ".join(cmds)
    code, env_outp, errp = __execute_script(cmds)
    if show_debug_output:
        print(f"output for {section_long_name}: {env_outp}")
    assert (
        code == 0
    ), f"Error getting generated {section_long_name}({code}): {raw_command}\n{env_outp}{errp}"
    return env_outp


def __print_formatted_lines(source_output_line_list, indent=""):
    for next_line in source_output_line_list:
        print(indent + next_line, end="")


def __create_job_start_text(args, is_normal_action):
    needs_text = ""
    env_text = "env:" if is_normal_action else ""
    if args.required_jobs:
        for next_job in args.required_jobs:
            if needs_text:
                needs_text += ", "
            needs_text += next_job
        needs_text = "needs: " + needs_text
        if env_text:
            needs_text += "\n    "
        else:
            needs_text += "\n"
    return (
        __JOB_PREFIX.replace("{needs}", needs_text)
        .replace("{name}", args.job_name)
        .replace("{env}", env_text)
    )


def __calculate_section_lines(cmd_options, job_name):
    section_line_map = {}

    section_line_map[__ENVIRONMENT_SECTION] = __collect_lines_for_section(
        __ENVIRONMENT_SECTION, "environment", cmd_options, job_name
    )
    section_line_map[__APT_SECTION] = __collect_lines_for_section(
        __APT_SECTION, "apt", cmd_options, job_name
    )
    section_line_map[__PIP_SECTION] = __collect_lines_for_section(
        __PIP_SECTION, "pip", cmd_options, job_name
    )
    section_line_map[__GIT_SECTION] = __collect_lines_for_section(
        __GIT_SECTION, "git", cmd_options, job_name
    )
    section_line_map[__WEB_SECTION] = __collect_lines_for_section(
        __WEB_SECTION, "web", cmd_options, job_name
    )
    section_line_map[__COPY_SECTION] = __collect_lines_for_section(
        __COPY_SECTION, "copy", cmd_options, job_name
    )
    section_line_map[__ARTIFACTS_SECTION] = __collect_lines_for_section(
        __ARTIFACTS_SECTION, "artifacts", cmd_options, job_name
    )
    section_line_map[__TESTS_SECTION] = __collect_lines_for_section(
        __TESTS_SECTION, "tests", cmd_options, job_name
    )
    section_line_map[__PACKAGE_SECTION] = __collect_lines_for_section(
        __PACKAGE_SECTION, "package", cmd_options, job_name
    )

    return section_line_map


def __install(run_outp, action):

    assert action.startswith(__INSTALL_ACTION_PREFIX)
    action = action[len(__INSTALL_ACTION_PREFIX) :]

    upload_items = []
    for next_line in run_outp:
        if next_line.startswith(":"):
            split_next_line = next_line[1:].split("=")
            upload_items.append(split_next_line)
        else:
            print(next_line, end="")
    for next_item_pair in upload_items:
        artifact_name = action + " " + next_item_pair[0]
        print(
            __INSTALL_SECTION_UPLOAD_TEMPLATE.replace("{name}", artifact_name).replace(
                "{path}", next_item_pair[1]
            )
        )


def __print_normal_prefix_lines(section_line_map, apt_outp, pip_outp):

    __print_formatted_lines(section_line_map[__ENVIRONMENT_SECTION], indent="      ")

    # Start of steps to `Install Required Applications`
    print(__STEPS_PREFIX_AND_APT_SECTION_HEADER, end="")
    print(apt_outp)

    # `Install Required Python Packages`
    if pip_outp:
        print(__PIP_SECTION_HEADER_TEMPLATE.replace("{pip}", pip_outp))

    # `Install Required Third Party Git Repositories`
    if section_line_map[__GIT_SECTION]:
        print(__GIT_SECTION_HEADER)
        __print_formatted_lines(section_line_map[__GIT_SECTION], indent="          ")

    # `Install Required Third Party Web Packages`
    if section_line_map[__WEB_SECTION]:
        print(__WEB_SECTION_HEADER)
        __print_formatted_lines(section_line_map[__WEB_SECTION], indent="          ")


# pylint: disable=too-many-arguments
def __print_next_section_core(
    args,
    next_section_cd,
    section_line_map,
    run_ordered_build_list,
    prerun_build_map,
    run_build_map,
):
    if (
        run_ordered_build_list[-1] == next_section_cd
        and section_line_map[__COPY_SECTION]
    ):
        print(__COPY_SECTION_HEADER)
        __print_formatted_lines(section_line_map[__COPY_SECTION], indent="          ")

    action_title = "Build" if args.action == __RUN_ACTION else args.action.capitalize()

    proper_section_name = next_section_cd[len("cd $GAIA_REPO/") :].strip()
    if prerun_build_map:
        if next_section_cd in prerun_build_map:
            print(
                __PREBUILD_SECTION_HEADER_TEMPLATE.replace(
                    "{action}", action_title
                ).replace("{section}", proper_section_name)
            )
            __print_formatted_lines(
                prerun_build_map[next_section_cd], indent="          "
            )
    print(
        __BUILD_SECTION_HEADER_TEMPLATE.replace("{action}", action_title).replace(
            "{section}", proper_section_name
        )
    )
    __print_formatted_lines(run_build_map[next_section_cd], indent="          ")


# pylint: enable=too-many-arguments


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    cmd_options = ["--directory", args.configuration_directory]
    for next_option in args.options:
        cmd_options.append("--option")
        cmd_options.append(next_option)

    section_line_map = __calculate_section_lines(cmd_options, args.job_name)

    # Run the munge script to create each part of the file.
    is_install_action = args.action.startswith(__INSTALL_ACTION_PREFIX)
    if is_install_action:
        prerun_outp = None
    else:
        prerun_outp = __collect_lines_for_section(
            "pre_" + args.action, "pre_" + args.action, cmd_options, args.job_name
        )
    run_outp = __collect_lines_for_section(
        args.action, args.action, cmd_options, args.job_name
    )

    # A small amount of adjustments to the input.
    assert len(section_line_map[__APT_SECTION]) == 1
    apt_outp = section_line_map[__APT_SECTION][0].strip()

    assert len(section_line_map[__PIP_SECTION]) == 1
    pip_outp = section_line_map[__PIP_SECTION][0].strip()

    prerun_build_map, _ = __create_build_map_and_ordered_list(prerun_outp)
    run_build_map, run_ordered_build_list = __create_build_map_and_ordered_list(
        run_outp
    )

    # Create the header for this particular job.
    print(__create_job_start_text(args, not is_install_action))
    if is_install_action:
        print(__INSTALL_SECTION_HEADER)
    else:
        __print_normal_prefix_lines(section_line_map, apt_outp, pip_outp)

    # Each of the subproject sections, up to `Build production``
    for next_section_cd in run_ordered_build_list:
        __print_next_section_core(
            args,
            next_section_cd,
            section_line_map,
            run_ordered_build_list,
            prerun_build_map,
            run_build_map,
        )

    # `Tests`
    if args.action == __RUN_ACTION:
        print(__TESTS_SECTION_HEADER)
        __print_formatted_lines(section_line_map[__TESTS_SECTION])

        # `Generate Package`
        __print_formatted_lines(section_line_map[__PACKAGE_SECTION])
    elif args.action == __LINT_ACTION:
        print(__LINT_SECTION_SUFFIX)
    elif is_install_action:
        __install(run_outp, args.action)

    if not is_install_action:
        # `Upload *`
        __print_formatted_lines(section_line_map[__ARTIFACTS_SECTION])
    print(__SLACK_SUFFIX)


sys.exit(process_script_action())

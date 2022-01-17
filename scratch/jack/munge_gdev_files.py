#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to munge the GDEV file tree and present information about each
of the individual sections.
"""

import sys
import pathlib
import os
import json
import argparse
import re

# The Gaia section is key in that it provides organization.
__SECTION_NAME_GAIA = "[gaia]"

# These sections are used by both GDev and GitHub Actions.
__SECTION_NAME_APT = "[apt]"
__SECTION_NAME_PIP = "[pip]"
__SECTION_NAME_GIT = "[git]"
__SECTION_NAME_ENV = "[env]"
__SECTION_NAME_WEB = "[web]"

__SECTION_NAME_PRE_RUN = "[pre_run]"
__SECTION_NAME_RUN = "[run]"
__SECTION_NAME_PRE_LINT = "[pre_lint]"
__SECTION_NAME_LINT = "[lint]"

# These sections are only used by GitHub Actions.
__SECTION_NAME_COPY = "[copy]"
__SECTION_NAME_ARTIFACTS = "[artifacts]"
__SECTION_NAME_PACKAGE = "[package]"
__SECTION_NAME_TESTS = "[tests]"
__SECTION_NAME_INSTALLS = "[installs]"

# Special section prefix.

__SECTION_NAME_INSTALL_PREFIX = "[install:"

# Lists of available object so we can validate at the command line.
__valid_section_names = [
    __SECTION_NAME_APT,
    __SECTION_NAME_PIP,
    __SECTION_NAME_GIT,
    __SECTION_NAME_WEB,
    __SECTION_NAME_ENV,
    __SECTION_NAME_COPY,
    __SECTION_NAME_ARTIFACTS,
    __SECTION_NAME_PACKAGE,
    __SECTION_NAME_TESTS,
    __SECTION_NAME_INSTALLS,
    __SECTION_NAME_PRE_RUN,
    __SECTION_NAME_RUN,
    __SECTION_NAME_PRE_LINT,
    __SECTION_NAME_LINT,
]
__available_sections = []

__available_options = [
    "GaiaSDK",
    "Debug",
    "GaiaLLVMTests",
    "ubuntu:20.04",
    "CI_GitHub",
    "Lint",
]

# Regular expressions that we support within {} constructs.
__SOURCE_DIR_REGEX = r"source_dir\(\'([^']+)\'\)"
__ENABLE_IF_REGEX = r"enable_if\(\'([^']+)\'\)"
__ENABLE_IF_NOT_REGEX = r"enable_if_not\(\'([^']+)\'\)"
__ENABLE_IF_ANY_REGEX = (
    r"enable_if_any\(\'([^']+)\'(?:\s*\,\s*\'([^']+)\')?(?:\s*\,\s*\'([^']+)\')?\)"
)
__ENABLE_IF_NONE_REGEX = (
    r"enable_if_not_any\(\'([^']+)\'(?:\s*\,\s*\'([^']+)\')?(?:\s*\,\s*\'([^']+)\')?\)"
)

# Templates to use for various task sections.
__PUBLISH_ARTIFACTS_TASK_TEMPLATE = """
      - name: Upload {name}
        if: always()
        uses: actions/upload-artifact@v2
        with:
          name: {job} {name}
          path: |"""

__PACKAGE_TASK_TEMPLATE = """
      - name: Generate Package
        run: |
{lines}

      - name: Upload Package
        uses: actions/upload-artifact@v2
        with:
          name: {job} Debian Install Package
          path: {path}"""

# Other Constants
__COMMENT_LINE_START_CHARACTER = "#"
__GDEV_NEW_SECTION_START_CHARACTER = "["
__GDEV_NEW_SECTION_END_CHARACTER = "]"
__GDEV_START_CONDITIONAL_EXPRESSION = "{"
__GDEV_END_CONDITIONAL_EXPRESSION = "}"
__GDEV_LINE_CONTINUATION_CHARACTER = "\\"

__GDEV_ENVIRONMENT_ITEM_SEPARATOR = "="
__GDEV_PACKAGE_PRODUCES_PREFIX = "produces:"
__GDEV_ARTIFACTS_ITEM_SEPARATOR = "="

__BUILD_CONFIGURATION_FILE_NAME = "gdev.cfg"
__GAIA_REPOSITORY_ENV_PATH = "$GAIA_REPO"
__SOURCE_DIRECTORY_PATH = "/source"
__BUILD_DIRECTORY_PATH = "/build"

__DEFAULT_FILE_ENCODING = "utf-8"


def __init():
    """
    Instead of duplicating the __valid_section_names and __available_sections
    variables, just fix them up here.
    """
    for next_section_name in __valid_section_names:
        __available_sections.append(next_section_name[1:-1])


def __valid_options(argument):
    """
    Function to help argparse limit the valid options that are supported.
    """
    if argument in __available_options:
        return argument
    raise ValueError(f"Value '{argument}' is not a valid option.")


def __valid_sections(argument):
    """
    Function to help argparse limit the valid sections that are supported.
    """
    if argument in __available_sections or argument.startswith(
        __SECTION_NAME_INSTALL_PREFIX[1:]
    ):
        return argument
    raise ValueError(f"Value '{argument}' is not a valid section name.")


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
        "--raw",
        dest="show_raw",
        action="store_true",
        help="Show the raw results of the scan and exit.",
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
        "--section",
        dest="section",
        action="store",
        help="Specify a section to output the collected results for.",
        type=__valid_sections,
        # choices=__available_sections,
    )
    parser.add_argument(
        "--job-name",
        dest="job_name",
        action="store",
        required=True,
        help="Name to give the job being created.",
    )
    parser.add_argument(
        "--verbose",
        dest="verbose_mode",
        action="store_true",
        help="Show extended information about what is going on.",
    )
    return parser.parse_args()


def __handle_gdev_conditional_line(next_line):
    """
    Take a given line as input, and return a pair containing that line and any conditional
    extracted from that line.
    """
    line_conditional = None

    if next_line.strip().startswith(__GDEV_START_CONDITIONAL_EXPRESSION):
        start_conditional_index = next_line.index(__GDEV_START_CONDITIONAL_EXPRESSION)
        end_conditional_index = next_line.index(__GDEV_END_CONDITIONAL_EXPRESSION)
        line_conditional = next_line[
            start_conditional_index + 1 : end_conditional_index
        ]
        next_line = (
            next_line[0:start_conditional_index]
            + next_line[end_conditional_index + 1 :]
        )
    return next_line, line_conditional


def __handle_new_section(next_line, loaded_sections, line_conditional):
    """
    Given a line that starts with the __GDEV_NEW_SECTION_START_CHARACTER sequence,
    take care of figuring out how to process the next section.

    Note that a conditional can be applied to a section, in which case, the
    section_conditional value is applied to any line that does not already have
    a conditional.
    """
    scan_error = None

    if (
        next_line not in __valid_section_names
        and next_line != __SECTION_NAME_GAIA
        and not next_line.startswith(__SECTION_NAME_INSTALL_PREFIX)
    ):
        scan_error = f"Unrecognized section name '{next_line}'."
        return None, None, scan_error
    section_conditional = line_conditional
    line_pairs = []
    loaded_sections[next_line] = line_pairs
    return section_conditional, line_pairs, None


def __calculate_conditional(
    next_line, line_conditional, following_conditional, section_conditional
):
    """
    With the multiple types of conditionals that can be applied, figure out which one has
    dominance, and return it.  The weakest of these is the following_conditional variable
    which is reset with each line.
    """

    if following_conditional and not line_conditional:
        line_conditional = following_conditional
    following_conditional = None

    if not line_conditional:
        line_conditional = section_conditional

    if next_line.endswith(__GDEV_LINE_CONTINUATION_CHARACTER):
        following_conditional = line_conditional
    return line_conditional, following_conditional


def __load_gdev_cfg(file_to_load):
    """
    Load the specified configuration file and parse it into its constituent parts.
    """
    loaded_sections, line_pairs, section_conditional, following_conditional = (
        {},
        [],
        None,
        None,
    )

    lines_in_text_file = (
        pathlib.Path(file_to_load)
        .read_text(encoding=__DEFAULT_FILE_ENCODING)
        .split("\n")
    )
    for next_line in lines_in_text_file:
        if (
            next_line.strip().startswith(__COMMENT_LINE_START_CHARACTER)
            or not next_line.strip()
        ):
            continue

        next_line, line_conditional = __handle_gdev_conditional_line(next_line)

        if next_line.startswith(__GDEV_NEW_SECTION_START_CHARACTER):
            section_conditional, line_pairs, scan_error = __handle_new_section(
                next_line, loaded_sections, line_conditional
            )
            if scan_error:
                return None, scan_error
            continue

        line_conditional, following_conditional = __calculate_conditional(
            next_line, line_conditional, following_conditional, section_conditional
        )

        line_pair = (next_line, line_conditional)
        line_pairs.append(line_pair)
    return loaded_sections, None


def __scan_all_configuration_files(root_file, root_path, args):
    """
    Starting from the root configuration file, find any configuration files that
    a configuration file depends on, and make sure it gets scanned and collected.
    """
    scanned_files, collected_file_sections, scan_error = set(), {}, None

    files_to_scan = [os.path.join(root_path, root_file)]
    while files_to_scan:
        next_file_to_scan = files_to_scan[0]
        del files_to_scan[0]
        if next_file_to_scan in scanned_files:
            continue

        scanned_files.add(next_file_to_scan)
        if args.verbose_mode:
            print(f"Scanning: {next_file_to_scan}")
        loaded_file_sections, scan_error = __load_gdev_cfg(next_file_to_scan)
        if scan_error:
            scan_error = (
                f"Error scanning configuration file '{next_file_to_scan}': {scan_error}"
            )
            return None, scan_error
        collected_file_sections[next_file_to_scan] = loaded_file_sections

        if __SECTION_NAME_GAIA in loaded_file_sections:
            for next_line_pair in loaded_file_sections[__SECTION_NAME_GAIA]:
                files_to_scan.append(
                    os.path.join(
                        root_path, next_line_pair[0], __BUILD_CONFIGURATION_FILE_NAME
                    )
                )

    return collected_file_sections, None


def __find_lowest_directory_without_configuration_file(base_config_file):
    """
    All paths in Gdev.cfg are relative to the "root" of the gdev "tree".  As such,
    keep on looking towards to root of the directory tree for a directory that does
    not contain a 'gdev.cfg' file.
    """

    next_directory = os.path.dirname(base_config_file)
    while True:
        check_file_name = os.path.join(next_directory, __BUILD_CONFIGURATION_FILE_NAME)
        if not os.path.exists(check_file_name):
            break
        next_directory = os.path.dirname(next_directory)
    return next_directory


def __initialize(args):
    """
    Initialize the processing of the gdev.cfg files.
    """

    if not os.path.isdir(args.configuration_directory):
        print(
            f"The specified path of '{args.configuration_directory}' is not a directory."
        )
        sys.exit(1)

    config_path = os.path.join(
        args.configuration_directory, __BUILD_CONFIGURATION_FILE_NAME
    )
    if not os.path.exists(config_path):
        print(
            f"A 'gdev.cfg' file does not exist at the expected path of {config_path}."
        )
        sys.exit(1)
    base_config_file = os.path.abspath(config_path)

    configuration_root_directory = __find_lowest_directory_without_configuration_file(
        base_config_file
    )
    rooted_file_name = base_config_file[len(configuration_root_directory) + 1 :]

    collected_file_sections, scan_error = __scan_all_configuration_files(
        rooted_file_name, configuration_root_directory, args
    )
    if scan_error:
        print(scan_error)
        sys.exit(1)

    return collected_file_sections, base_config_file, configuration_root_directory


def __does_meet_project_options(conditional_to_evaluate, active_options):
    """
    Given a conditional and options to match against, determine if the conditional
    evaluates to true.
    """

    if not conditional_to_evaluate:
        return True

    provided_options_set = set(active_options) if active_options else set()
    match_result = re.match(__ENABLE_IF_ANY_REGEX, conditional_to_evaluate)
    if match_result:
        matched_groups_set = set(match_result.groups())
        return bool(matched_groups_set.intersection(provided_options_set))

    match_result = re.match(__ENABLE_IF_NONE_REGEX, conditional_to_evaluate)
    if match_result:
        matched_groups_set = set(match_result.groups())
        return not bool(matched_groups_set.intersection(provided_options_set))

    match_result = re.match(__ENABLE_IF_REGEX, conditional_to_evaluate)
    if match_result:
        matched_groups_set = set(match_result.groups())
        return bool(matched_groups_set.intersection(provided_options_set))

    match_result = re.match(__ENABLE_IF_NOT_REGEX, conditional_to_evaluate)
    if match_result:
        matched_groups_set = set(match_result.groups())
        return not bool(matched_groups_set.intersection(provided_options_set))

    print(f"Conditional '{conditional_to_evaluate}' not supported.")
    sys.exit(1)


def __check_if_project_dependencies_already_processed(
    files_to_search_in, already_searched_files
):
    """
    Determine if there is another project dependencies to process.
    """
    next_project_to_process = files_to_search_in[0]

    del files_to_search_in[0]
    if next_project_to_process in already_searched_files:
        return None
    already_searched_files.add(next_project_to_process)
    return next_project_to_process


def __calculate_project_dependencies(
    base_config_file, collected_file_sections, configuration_root_directory, args
):
    """
    Calculate the dependencies for the project one file at a time.
    """
    (
        project_dependencies,
        dependency_graph,
        files_to_search_in,
        already_searched_files,
    ) = ([base_config_file], {}, [base_config_file], set())

    while files_to_search_in:
        next_project_config = __check_if_project_dependencies_already_processed(
            files_to_search_in, already_searched_files
        )
        if not next_project_config:
            continue

        project_dependency_list = []
        dependency_graph[next_project_config] = project_dependency_list
        if __SECTION_NAME_GAIA in collected_file_sections[next_project_config]:
            gaia_dependencies = collected_file_sections[next_project_config][
                __SECTION_NAME_GAIA
            ]
            for next_project_directory in gaia_dependencies:
                if __does_meet_project_options(next_project_directory[1], args.options):
                    next_project_dependency = os.path.join(
                        configuration_root_directory,
                        next_project_directory[0],
                        __BUILD_CONFIGURATION_FILE_NAME,
                    )

                    project_dependency_list.append(next_project_dependency)

                    # Even though we checked to make sure we do not start processing the same
                    # project twice, multiple projects may still reference the same dependent
                    # project.
                    if next_project_dependency not in project_dependencies:
                        project_dependencies.append(next_project_dependency)
                        files_to_search_in.append(next_project_dependency)

    return project_dependencies, dependency_graph


# pylint: disable=too-many-arguments, too-many-locals
def __collect_specified_section_text_pairs(
    specific_section_name,
    alternate_section_name,
    base_config_file,
    collected_file_sections,
    configuration_root_directory,
    args,
):
    """
    For the specified section, calculate the dependencies and the text pairs for each configuration
    file that has one of those sections.
    """
    collected_text_pairs = []

    project_dependencies, dependency_graph = __calculate_project_dependencies(
        base_config_file, collected_file_sections, configuration_root_directory, args
    )
    for next_project_dependency in project_dependencies:

        test_section_name = specific_section_name
        if test_section_name not in collected_file_sections[next_project_dependency]:
            test_section_name = alternate_section_name

        if test_section_name in collected_file_sections[next_project_dependency]:
            spec_dependencies = collected_file_sections[next_project_dependency][
                test_section_name
            ]
            for next_spec in spec_dependencies:
                if next_spec[1] and next_spec[1].startswith("source_dir("):
                    match_result = re.match(__SOURCE_DIR_REGEX, next_spec[1])
                    if not match_result:
                        sys.exit(1)
                    adjusted_path = os.path.join(
                        __SOURCE_DIRECTORY_PATH, match_result.groups()[0]
                    )
                    origin_spec_pair = (next_project_dependency, adjusted_path)
                    collected_text_pairs.append(origin_spec_pair)
                else:
                    if __does_meet_project_options(next_spec[1], args.options):
                        origin_spec_pair = (next_project_dependency, next_spec[0])
                        collected_text_pairs.append(origin_spec_pair)
    return collected_text_pairs, dependency_graph


# pylint: enable=too-many-arguments, too-many-locals


def __adjust_script_lines_for_sudo(next_line):
    """
    Given a normal script line, prepare it for inclusion in one of the returned sections.

    Note: for historical reasons, there are dependencies in the scripts that rely on
    the project being built with sudo permissions.  This function takes care of applying
    that wrapper on the lines unless it is explicitly not needed.
    """

    if not next_line.startswith("cd "):
        next_line = 'sudo bash -c "' + next_line.replace('"', '\\"') + '"'
    next_line = "          " + next_line
    return next_line


def __print_apt_and_pip_results(section_text_pairs):
    """
    Print the text to represent the 'apt' and 'pip' sections in a composed manner.
    """

    for next_pair in section_text_pairs:
        print(f" {next_pair[1]}", end="")
    print("")


def __print_git_results(section_text_pairs, configuration_root_directory):
    """
    Print the script lines required to satisfy any Git subproject requirements.
    """

    for next_pair in section_text_pairs:
        project_directory = os.path.dirname(
            next_pair[0].replace(
                configuration_root_directory, __GAIA_REPOSITORY_ENV_PATH
            )
        )
        print(f"cd {project_directory}")
        print(f"git clone -c advice.detachedHead=false --depth 1 {next_pair[1]}")
        print("rm -rf */.git")


def __print_env_results(section_text_pairs):
    """
    Print colon separated pairs of values used to specify environment variables.
    """

    print("GAIA_REPO: ${{ github.workspace }}")
    for next_pair in section_text_pairs:
        split_pair = next_pair[1].split(__GDEV_ENVIRONMENT_ITEM_SEPARATOR, 1)
        print(f"{split_pair[0]}: {split_pair[1]}")


def __print_web_results(section_text_pairs, configuration_root_directory):
    """
    Print the script lines required to satisfy any subproject requirements that need
    to be explicitly downloaded from the web.
    """

    for next_pair in section_text_pairs:
        project_directory = os.path.dirname(
            next_pair[0].replace(
                configuration_root_directory, __GAIA_REPOSITORY_ENV_PATH
            )
        )
        print(f"cd {project_directory}")
        print(f"wget {next_pair[1]}")


def __build_ordered_dependency_list(dependency_graph):
    """
    Build an ordered list of dependencies that can be followed as a guide to
    which subprojects to visit in which order.
    """

    ordered_dependencies = []
    while dependency_graph:

        fully_resolved_configs = []
        for config_path in dependency_graph:

            satisified_dependencies_to_remove = []
            for dependency_for_config_path in dependency_graph[config_path]:
                if dependency_for_config_path in ordered_dependencies:
                    satisified_dependencies_to_remove.append(dependency_for_config_path)

            for dependency_to_remove in satisified_dependencies_to_remove:
                satisified_dependencies_to_remove_index = dependency_graph[
                    config_path
                ].index(dependency_to_remove)
                del dependency_graph[config_path][
                    satisified_dependencies_to_remove_index
                ]

            if not dependency_graph[config_path]:
                fully_resolved_configs.append(config_path)

        if not fully_resolved_configs:
            print("One or more configuration dependencies are not resolvable.")
            sys.exit(1)
        for next_fully_resolved_config in fully_resolved_configs:
            del dependency_graph[next_fully_resolved_config]
            ordered_dependencies.append(next_fully_resolved_config)

    return ordered_dependencies


def __calculate_lines_per_section(section_text_pairs, configuration_root_directory):
    """
    For each line in a section pair, create a new map which uses the first item in
    the pair, the name of the configuration section, as the key.
    """
    current_directory = None
    total_line = ""

    lines_per_section = {}
    for next_pair in section_text_pairs:
        current_section_line = ""
        if next_pair[0] in lines_per_section:
            current_section_line = lines_per_section[next_pair[0]]
        else:
            lines_per_section[next_pair[0]] = ""
        project_directory = os.path.dirname(
            next_pair[0].replace(
                configuration_root_directory, __GAIA_REPOSITORY_ENV_PATH
            )
        )
        if current_directory != next_pair[0]:
            current_section_line += f"\ncd {project_directory}\n"
            current_directory = next_pair[0]

        next_line = next_pair[1]
        if next_line.endswith(__GDEV_LINE_CONTINUATION_CHARACTER):
            total_line += next_line[:-1]
        else:
            total_line += next_line
            if total_line.startswith("cd "):
                current_section_line += total_line.replace('"', '\\"') + "\n"
            else:
                current_section_line += (
                    'sudo bash -c "' + total_line.replace('"', '\\"') + '"' + "\n"
                )
            total_line = ""
        lines_per_section[next_pair[0]] = current_section_line
    return lines_per_section


def __print_run_and_pre_run_results(
    section_text_pairs, configuration_root_directory, dependency_graph
):
    """
    Using an ordered list of dependencies as a guide, output both the pre-run and run
    results in that ordered manner.
    """

    lines_per_section = __calculate_lines_per_section(
        section_text_pairs, configuration_root_directory
    )
    ordered_dependencies = __build_ordered_dependency_list(dependency_graph)
    for next_dependency in ordered_dependencies:
        if next_dependency in lines_per_section:
            print(lines_per_section[next_dependency])


def __depth_first_visit_project_directories(
    collected_file_sections,
    current_config_file,
    args,
    configuration_root_directory,
    visited_project_directories,
):
    """
    Recursive function to visit the project directories in a depth first manner.

    Part of the reason for the depth-first is historic, the other is practical.
    The practical reason is that it always guarantees that and dependencies for a
    given subproject are satisfied first.
    """

    current_configuration_sections = collected_file_sections[current_config_file]
    if __SECTION_NAME_GAIA in current_configuration_sections:
        this_gaia_section = current_configuration_sections[__SECTION_NAME_GAIA]
        for next_project_directory in this_gaia_section:
            if __does_meet_project_options(next_project_directory[1], args.options):
                sub_project_directory = os.path.join(
                    configuration_root_directory, next_project_directory[0]
                )
                if sub_project_directory not in visited_project_directories:
                    sub_project_config_file = os.path.join(
                        sub_project_directory, __BUILD_CONFIGURATION_FILE_NAME
                    )
                    __depth_first_visit_project_directories(
                        collected_file_sections,
                        sub_project_config_file,
                        args,
                        configuration_root_directory,
                        visited_project_directories,
                    )
                    visited_project_directories.append(sub_project_directory)


def __calculate_directories_to_copy_to(current_sections):
    """
    Calculate which of the build and source directories from a given subproject should be
    copied to the main /build and /source directories.
    """
    copy_to_build, copy_to_source = False, False
    if __SECTION_NAME_COPY in current_sections:
        for copy_section_entry in current_sections[__SECTION_NAME_COPY]:
            section_entry = copy_section_entry[0].strip()
            if not section_entry:
                continue
            if section_entry == "build":
                copy_to_build = True
            elif section_entry == __SOURCE_DIRECTORY_PATH[1:]:
                copy_to_source = True
            elif section_entry != "none":
                print(
                    f"Section {__SECTION_NAME_COPY} entry of '{section_entry}' not supported."
                )
                sys.exit(1)
    else:
        if __SECTION_NAME_RUN in current_sections:
            copy_to_build = True
        else:
            copy_to_source = True
    return copy_to_build, copy_to_source


def __print_pre_production_copy_section(
    collected_file_sections, base_config_file, args, configuration_root_directory
):
    """
    Print any copy information required to move the source and build data from
    their current locations into the /build and /source directories for the major
    build task.
    """
    visited_project_directories = []
    __depth_first_visit_project_directories(
        collected_file_sections,
        base_config_file,
        args,
        configuration_root_directory,
        visited_project_directories,
    )
    visited_project_directories.append(os.path.dirname(base_config_file))
    for next_visited_directory in visited_project_directories:
        visisted_directory_config_file = os.path.join(
            next_visited_directory, __BUILD_CONFIGURATION_FILE_NAME
        )
        current_sections = collected_file_sections[visisted_directory_config_file]
        assert visisted_directory_config_file.startswith(configuration_root_directory)
        relative_config_file_path = visisted_directory_config_file[
            len(configuration_root_directory) : -(
                len(__BUILD_CONFIGURATION_FILE_NAME) + 1
            )
        ]

        copy_to_build, copy_to_source = __calculate_directories_to_copy_to(
            current_sections
        )

        if copy_to_build:
            print(
                f'sudo bash -c "mkdir -p {__BUILD_DIRECTORY_PATH}{relative_config_file_path}"'
            )
            print(
                f'sudo bash -c "cp -a {__GAIA_REPOSITORY_ENV_PATH}{relative_config_file_path}/.'
                + f' {__BUILD_DIRECTORY_PATH}{relative_config_file_path}/"'
            )
        if copy_to_source:
            print(
                f'sudo bash -c "mkdir -p {__SOURCE_DIRECTORY_PATH}{relative_config_file_path}"'
            )
            print(
                f'sudo bash -c "cp -a {__GAIA_REPOSITORY_ENV_PATH}{relative_config_file_path}/.'
                + f' {__SOURCE_DIRECTORY_PATH}{relative_config_file_path}/"'
            )


def __print_artifacts_section(section_text_pairs, job_name):
    """
    Print the information from the artifacts section of the main gdev.cfg.
    """

    last_artifact_name = None
    for next_text_pair in section_text_pairs:
        split_artifact_data = next_text_pair[1].split(
            __GDEV_ARTIFACTS_ITEM_SEPARATOR, 1
        )
        if split_artifact_data[0] != last_artifact_name:
            print(
                __PUBLISH_ARTIFACTS_TASK_TEMPLATE.replace(
                    "{name}", split_artifact_data[0]
                ).replace("{job}", job_name)
            )
            last_artifact_name = split_artifact_data[0]
        print("            " + split_artifact_data[1])


def __print_package_section(section_text_pairs, job_name):
    """
    Print the script lines associated with creating a package.

    Note that this section is an optional section, so it is valid to
    have no data here at all.

    Note that the first line must begin with the 'produces:' prefix
    to specify the name of the file that will be produced by the packaging
    command.
    """
    if not section_text_pairs:
        return

    assert len(section_text_pairs) >= 2
    section_first_line = section_text_pairs[0][1]
    assert section_first_line.startswith(__GDEV_PACKAGE_PRODUCES_PREFIX)
    package_path = section_first_line[len(__GDEV_PACKAGE_PRODUCES_PREFIX) :]
    package_lines = []
    for next_line_pair_index in range(1, len(section_text_pairs)):
        next_line = section_text_pairs[next_line_pair_index][1]
        package_lines.append(__adjust_script_lines_for_sudo(next_line))

    print(
        __PACKAGE_TASK_TEMPLATE.replace("{lines}", "\n".join(package_lines))
        .replace("{path}", package_path)
        .replace("{job}", job_name)
    )


def __print_tests_section(section_text_pairs):
    """
    Print the scripts lines associated with the root [tests] section.
    """

    for next_line_pair in section_text_pairs:
        next_line = next_line_pair[1]
        print(__adjust_script_lines_for_sudo(next_line))


def __print_installs_section(section_text_pairs):
    """
    Print the scripts lines from the [installs] section.  Note that this
    is primarily used as a guide to figure out which [install:xxx] section
    are to be used.
    """
    for next_line_pair in section_text_pairs:
        next_line = next_line_pair[1]
        print(next_line)


def __print_specific_installs_section(section_text_pairs):
    """
    Print the scripts line associated with a specific [install:xxx] section.
    Note that lines starting with ':' are passed as is because the other
    scripts will interpret them.
    """
    for next_line_pair in section_text_pairs:
        next_line = next_line_pair[1]
        if next_line.startswith(":"):
            print(next_line)
        else:
            print(__adjust_script_lines_for_sudo(next_line))


# pylint: disable=too-many-branches
def process_script_action():
    """
    Process the request to provide aggregated information on a specific section
    of a tree of Gdev.cfg files.
    """
    __init()
    args = __process_command_line()
    (
        collected_file_sections,
        base_config_file,
        configuration_root_directory,
    ) = __initialize(args)

    if args.show_raw:
        print(json.dumps(collected_file_sections, indent=2))
        return 0
    if not args.section:
        print("Either the --raw argument or a --section argument must be specified.")
        return 1

    specific_section_name = (
        __GDEV_NEW_SECTION_START_CHARACTER
        + args.section
        + __GDEV_NEW_SECTION_END_CHARACTER
    )
    if specific_section_name == __SECTION_NAME_PRE_LINT:
        alternate_section_name = __SECTION_NAME_PRE_RUN
    elif specific_section_name == __SECTION_NAME_LINT:
        alternate_section_name = __SECTION_NAME_RUN
    else:
        alternate_section_name = None

    section_text_pairs, dependency_graph = __collect_specified_section_text_pairs(
        specific_section_name,
        alternate_section_name,
        base_config_file,
        collected_file_sections,
        configuration_root_directory,
        args,
    )

    if specific_section_name in (__SECTION_NAME_APT, __SECTION_NAME_PIP):
        __print_apt_and_pip_results(section_text_pairs)
    elif __SECTION_NAME_GIT == specific_section_name:
        __print_git_results(section_text_pairs, configuration_root_directory)
    elif __SECTION_NAME_ENV == specific_section_name:
        __print_env_results(section_text_pairs)
    elif __SECTION_NAME_WEB == specific_section_name:
        __print_web_results(section_text_pairs, configuration_root_directory)
    elif __SECTION_NAME_COPY == specific_section_name:
        __print_pre_production_copy_section(
            collected_file_sections,
            base_config_file,
            args,
            configuration_root_directory,
        )
    elif __SECTION_NAME_ARTIFACTS == specific_section_name:
        __print_artifacts_section(section_text_pairs, args.job_name)
    elif __SECTION_NAME_PACKAGE == specific_section_name:
        __print_package_section(section_text_pairs, args.job_name)
    elif __SECTION_NAME_TESTS == specific_section_name:
        __print_tests_section(section_text_pairs)
    elif __SECTION_NAME_INSTALLS == specific_section_name:
        __print_installs_section(section_text_pairs)
    elif specific_section_name.startswith(__SECTION_NAME_INSTALL_PREFIX):
        __print_specific_installs_section(section_text_pairs)
    else:
        assert specific_section_name in (
            __SECTION_NAME_RUN,
            __SECTION_NAME_PRE_RUN,
            __SECTION_NAME_LINT,
            __SECTION_NAME_PRE_LINT,
        )
        __print_run_and_pre_run_results(
            section_text_pairs, configuration_root_directory, dependency_graph
        )

    return 0


# pylint: enable=too-many-branches


sys.exit(process_script_action())

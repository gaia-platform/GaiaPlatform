#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from logging import fatal
import sys
import io
import pathlib
import os
import json
import argparse
import re

__section_name_gaia = "[gaia]"

__section_name_apt = "[apt]"
__section_name_pip = "[pip]"
__section_name_git = "[git]"
__section_name_env = "[env]"
__section_name_web = "[web]"
__section_name_pre_run = "[pre_run]"
__section_name_run = "[run]"

__valid_section_names = [__section_name_apt, __section_name_gaia, __section_name_pip, __section_name_pre_run, "[run]", __section_name_git, __section_name_web, __section_name_env]

__available_sections = [ __section_name_apt[1:-1], __section_name_pip[:1:-1], __section_name_git[1:-1],
    __section_name_env[1:-1], __section_name_web[1:-1], __section_name_pre_run[1:-1], __section_name_run[1:-1]]

__available_options = ["GaiaRelease", "ubuntu:20.04"]

source_dir_regex = r"source_dir\(\'([^']+)\'\)"
enable_if_regex = r"enable_if\(\'([^']+)\'\)"
enable_if_not_regex = r"enable_if_not\(\'([^']+)\'\)"
enable_if_any_regex = r"enable_if_any\(\'([^']+)\'(?:\s*\,\s*\'([^']+)\')?(?:\s*\,\s*\'([^']+)\')?\)"


__comment_line_start_character = "#"
__build_configuration_file_name = "gdev.cfg"


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
    if argument in __available_sections:
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
        choices=__available_sections,
    )
    parser.add_argument(
        "--verbose",
        dest="verbose_mode",
        action="store_true",
        help="Show extended information about what is going on.",
    )
    return parser.parse_args()

def __handle_conditional_line(next_line):
    line_conditional = None

    if next_line.strip().startswith("{"):
        start_conditional_index = next_line.index("{")
        end_conditional_index = next_line.index("}")
        line_conditional = next_line[start_conditional_index+1:end_conditional_index]
        next_line = next_line[0:start_conditional_index] + next_line[end_conditional_index + 1:]
    return next_line, line_conditional

def __handle_new_section(next_line, loaded_sections, line_conditional):
    scan_error = None

    if next_line not in __valid_section_names:
        scan_error = f"Unrecognized section name '{next_line}'."
        return None, None, scan_error
    section_conditional = line_conditional
    line_pairs = []
    loaded_sections[next_line] = line_pairs
    return section_conditional, line_pairs, None

def __calculate_conditional(next_line, line_conditional, following_conditional, section_conditional):

    if following_conditional and not line_conditional:
        line_conditional = following_conditional
    following_conditional = None
    if not line_conditional:
        line_conditional = section_conditional
    if next_line.endswith("\\"):
        following_conditional = line_conditional
    return line_conditional, following_conditional

def load_gdev_cfg(file_to_load):

    lines_in_text_file = pathlib.Path(file_to_load).read_text().split("\n")
    loaded_sections, line_pairs, section_conditional, following_conditional = {}, [], None, None

    for next_line in lines_in_text_file:
        if next_line.strip().startswith(__comment_line_start_character) or not next_line.strip():
            continue

        next_line, line_conditional = __handle_conditional_line(next_line)

        if next_line.startswith("["):
            section_conditional, line_pairs, scan_error = __handle_new_section(next_line, loaded_sections, line_conditional)
            if scan_error:
                return None, scan_error
            continue

        line_conditional, following_conditional = \
            __calculate_conditional(next_line, line_conditional, following_conditional, section_conditional)

        line_pair = (next_line, line_conditional)
        line_pairs.append(line_pair)
    return loaded_sections, None

def __scan_all_configuration_files(root_file, root_path, args):
    scanned_files = set()
    collected_file_sections = {}
    scan_error = None
    files_to_scan = [os.path.join(root_path, root_file)]

    while files_to_scan:
        next_file_to_scan = files_to_scan[0]
        del files_to_scan[0]
        if next_file_to_scan in scanned_files:
            continue

        scanned_files.add(next_file_to_scan)
        if args.verbose_mode:
            print("Scanning: " + next_file_to_scan)
        loaded_file_sections, scan_error = load_gdev_cfg(next_file_to_scan)
        if scan_error:
            scan_error = f"Error scanning configuration file '{next_file_to_scan}': {scan_error}"
            return None, scan_error
        collected_file_sections[next_file_to_scan] = loaded_file_sections

        if __section_name_gaia in loaded_file_sections:
            for next_line_pair in loaded_file_sections[__section_name_gaia]:
                files_to_scan.append(os.path.join(root_path, next_line_pair[0], __build_configuration_file_name))

    return collected_file_sections, None

def __find_lowest_directory_without_configuration_file(base_config_file):
    next_directory = os.path.dirname(base_config_file)

    while True:
        check_file_name = os.path.join(next_directory, __build_configuration_file_name)
        if not os.path.exists(check_file_name):
            break
        next_directory = os.path.dirname(next_directory)
    return next_directory

def __initialize(args):

    if not os.path.isdir(args.configuration_directory):
        print("not directory: " + args.configuration_directory)
        sys.exit(1)

    config_path = os.path.join(args.configuration_directory, __build_configuration_file_name)
    if not os.path.exists(config_path):
        print("not exist: " + config_path)
        sys.exit(1)
    base_config_file = os.path.abspath(config_path)

    configuration_root_directory = __find_lowest_directory_without_configuration_file(base_config_file)
    rooted_file_name = base_config_file[len(configuration_root_directory) + 1:]

    collected_file_sections, scan_error = __scan_all_configuration_files(rooted_file_name, configuration_root_directory, args)
    if scan_error:
        print(scan_error)
        sys.exit(1)

    return collected_file_sections, base_config_file, configuration_root_directory

def __does_meet_project_requirements(conditional_to_evaluate, active_options):
    provided_options_set = set(active_options)

    if not conditional_to_evaluate:
        return True
    match_result = re.match(enable_if_any_regex, conditional_to_evaluate)
    if match_result:
        matched_groups_set = set(match_result.groups())
        return bool(matched_groups_set.intersection(provided_options_set))
    match_result = re.match(enable_if_regex, conditional_to_evaluate)
    if match_result:
        matched_groups_set = set(match_result.groups())
        return bool(matched_groups_set.intersection(provided_options_set))
    match_result = re.match(enable_if_not_regex, conditional_to_evaluate)
    if match_result:
        matched_groups_set = set(match_result.groups())
        return not bool(matched_groups_set.intersection(provided_options_set))

    print(f"Conditional '{conditional_to_evaluate}' not supported.")
    sys.exit(1)

def __check_if_project_already_processed(files_to_search_in, already_searched_files):
    next_project_to_process = files_to_search_in[0]

    del files_to_search_in[0]
    if next_project_to_process in already_searched_files:
        return None
    already_searched_files.add(next_project_to_process)
    return next_project_to_process

def __calculate_project_dependencies(base_config_file, collected_file_sections, configuration_root_directory, args):
    project_dependencies = [base_config_file]
    files_to_search_in, already_searched_files = [base_config_file], set()

    dependency_graph = {}
    while files_to_search_in:
        next_project_config = __check_if_project_already_processed(files_to_search_in, already_searched_files)
        if not next_project_config:
            continue
        project_dependency_list = []
        dependency_graph[next_project_config] = project_dependency_list
        if __section_name_gaia in collected_file_sections[next_project_config]:
            gaia_dependencies = collected_file_sections[next_project_config][__section_name_gaia]
            for next_project_directory in gaia_dependencies:

                if __does_meet_project_requirements(next_project_directory[1], args.options):
                    next_project_dependency = os.path.join(configuration_root_directory,
                        next_project_directory[0],
                        __build_configuration_file_name)

                    project_dependency_list.append(next_project_dependency)

                    # Even though we checked to make sure we do not start processing the same
                    # project twice, multiple projects may still reference the same dependent
                    # project.
                    if next_project_dependency not in project_dependencies:
                        project_dependencies.append(next_project_dependency)
                        files_to_search_in.append(next_project_dependency)

    return project_dependencies, dependency_graph

def __collect_specified_section_text_pairs(specific_section_name, base_config_file, collected_file_sections, configuration_root_directory, args):
    collected_text_pairs = []

    project_dependencies, dependency_graph = \
        __calculate_project_dependencies(base_config_file, collected_file_sections, configuration_root_directory, args)
    for next_project_dependency in project_dependencies:
        if specific_section_name in collected_file_sections[next_project_dependency]:
            spec_dependencies = collected_file_sections[next_project_dependency][specific_section_name]
            for next_spec in spec_dependencies:
                if next_spec[1] and next_spec[1].startswith("source_dir("):
                    match_result = re.match(source_dir_regex, next_spec[1])
                    if not match_result:
                        sys.exit(1)
                    origin_spec_pair = (next_project_dependency, "$GAIA_REPO/" + match_result.groups()[0])
                    collected_text_pairs.append(origin_spec_pair)
                else:
                    if __does_meet_project_requirements(next_spec[1], args.options):
                        origin_spec_pair = (next_project_dependency, next_spec[0])
                        collected_text_pairs.append(origin_spec_pair)
    return collected_text_pairs, dependency_graph

def __print_apt_and_pip_results(section_text_pairs):

    for next_pair in section_text_pairs:
        print(f" {next_pair[1]}", end="")
    print("")

def __print_git_results(section_text_pairs, configuration_root_directory):

    for next_pair in section_text_pairs:
        project_directory = os.path.dirname(next_pair[0].replace(configuration_root_directory, "$GAIA_REPO"))
        print("cd " + project_directory)
        print("git clone -c advice.detachedHead=false --depth 1 " + next_pair[1])
        print("rm -rf */.git")

def __print_env_results(section_text_pairs):

    print("GAIA_REPO: /home/runner/work/GaiaPlatform/GaiaPlatform")
    for next_pair in section_text_pairs:
        split_pair = next_pair[1].split("=", 1)
        print(split_pair[0] + ": " + split_pair[1])

def __print_web_results(section_text_pairs, configuration_root_directory):

    for next_pair in section_text_pairs:
        project_directory = os.path.dirname(next_pair[0].replace(configuration_root_directory, "$GAIA_REPO"))
        print("cd " + project_directory)
        print("wget " + next_pair[1])

def __build_ordered_dependency_list(dependency_graph):
    ordered_dependencies = []
    while dependency_graph:

        fully_resolved_configs = []
        for config_path in dependency_graph:

            satisified_dependencies_to_remove = []
            for dependency_for_config_path in dependency_graph[config_path]:
                if dependency_for_config_path in ordered_dependencies:
                    satisified_dependencies_to_remove.append(dependency_for_config_path)

            for dependency_to_remove in satisified_dependencies_to_remove:
                satisified_dependencies_to_remove_index = dependency_graph[config_path].index(dependency_to_remove)
                del dependency_graph[config_path][satisified_dependencies_to_remove_index]

            if not dependency_graph[config_path]:
                fully_resolved_configs.append(config_path)

        if not fully_resolved_configs:
            print("One or more configuration dependencies are not resolvable.")
            sys.exit(1)
        for next_fully_resolved_config in fully_resolved_configs:
            del dependency_graph[next_fully_resolved_config]
            ordered_dependencies.append(next_fully_resolved_config)

    return ordered_dependencies

def __xx(section_text_pairs, configuration_root_directory):
    current_directory = None
    total_line = ""

    lines_per_section = {}
    for next_pair in section_text_pairs:
        current_section_line = ""
        if next_pair[0] in lines_per_section:
            current_section_line = lines_per_section[next_pair[0]]
        else:
            lines_per_section[next_pair[0]] = ""
        project_directory = os.path.dirname(next_pair[0].replace(configuration_root_directory, "$GAIA_REPO"))
        if current_directory != next_pair[0]:
            current_section_line += "\ncd " + project_directory + "\n"
            current_directory = next_pair[0]

        next_line = next_pair[1]
        if next_line.endswith("\\"):
            total_line += next_line[:-1]
        else:
            total_line += next_line
            if total_line.startswith("cd "):
                current_section_line += total_line.replace("\"", "\\\"") + "\n"
            else:
                current_section_line += "sudo bash -c \"" + total_line.replace("\"", "\\\"") + "\"" + "\n"
            total_line = ""
        lines_per_section[next_pair[0]] = current_section_line
    return lines_per_section

def __print_run_and_pre_run_results(section_text_pairs, configuration_root_directory, dependency_graph):
    lines_per_section =__xx(section_text_pairs, configuration_root_directory)

    ordered_dependencies = __build_ordered_dependency_list(dependency_graph)
    print("ordered_dependencies=" + str(ordered_dependencies))

    for i in ordered_dependencies:
        if i in lines_per_section:
            print(lines_per_section[i])

def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()
    collected_file_sections, base_config_file, configuration_root_directory = __initialize(args)

    if args.show_raw:
        print(json.dumps(collected_file_sections, indent=2))
        return 0
    assert args.section

    specific_section_name = "[" + args.section + "]"

    section_text_pairs, dependency_graph = __collect_specified_section_text_pairs(specific_section_name, base_config_file, collected_file_sections, configuration_root_directory, args)
    if __section_name_apt == specific_section_name or __section_name_pip == specific_section_name:
        __print_apt_and_pip_results(section_text_pairs)
    elif __section_name_git == specific_section_name:
        __print_git_results(section_text_pairs, configuration_root_directory)
    elif __section_name_env == specific_section_name:
        __print_env_results(section_text_pairs)
    elif __section_name_web == specific_section_name:
        __print_web_results(section_text_pairs, configuration_root_directory)
    # elif __section_name_pre_run == specific_section_name:
    #     gh = None
    #     for i in xx:
    #         fg = os.path.dirname(i[0].replace(configuration_root_directory, "$GAIA_REPO"))
    #         if gh != i[0]:
    #             print("cd " + fg)
    #             gh = i[0]
    #         print(str(i[1]))
    else:
        assert __section_name_run == specific_section_name or __section_name_pre_run == specific_section_name
        __print_run_and_pre_run_results(section_text_pairs, configuration_root_directory, dependency_graph)
    return 0

sys.exit(process_script_action())

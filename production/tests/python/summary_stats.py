#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Script to translate a test's output.json file into a output.csv file.
"""

import json
import sys
import os
import math
import argparse
from enum import Enum

DECIMALS_PLACES_IN_MILLISECONDS = 3

__PLAIN_FORMAT = "plain"
__JSON_FORMAT = "json"

__DEFAULT_FILE_ENCODING = "utf-8"


def __check_output_format(value):
    if value not in (__PLAIN_FORMAT, __JSON_FORMAT):
        raise argparse.ArgumentTypeError(
            f"Parameter {value} must be one of: {__PLAIN_FORMAT}, {__JSON_FORMAT}."
        )
    return value


def __process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Generate statistics based on the suite that was executed."
    )
    parser.add_argument(
        "--format",
        dest="output_format",
        action="store",
        type=__check_output_format,
        default=__PLAIN_FORMAT,
        help="Format to use for the output.",
    )
    parser.add_argument(
        "--workload",
        dest="workload_name",
        action="store",
        help="Workload that the test to sumarize falls under.",
    )
    return parser.parse_args()


def __translate_return_code(return_code):
    value = "unknown"
    if not return_code:
        value = "Success"
    elif return_code == 1:
        value = "SuiteFailure"
    elif return_code == 5:
        value = "CriticalFailure"
    elif return_code == 5:
        value = "TestExecutionFailure"
    elif return_code == 6:
        value = "ExpectedComparisonFailure"
    else:
        value = f"Failure:{return_code}"
    return value


# pylint: disable=invalid-name
def percentile(N, percent, key=lambda x: x):
    """
    Find the percentile of a list of values.

    @parameter N - is a list of values. Note N MUST BE already sorted.
    @parameter percent - a float value from 0.0 to 1.0.
    @parameter key - optional key function to compute value from each element of N.

    @return - the percentile of the values

    https://stackoverflow.com/questions/2374640/how-do-i-calculate-percentiles-with-python-numpy
    """
    if not N:
        return None
    k = (len(N) - 1) * percent
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return key(N[int(k)])
    d0 = key(N[int(f)]) * (c - k)
    d1 = key(N[int(c)]) * (k - f)
    return d0 + d1


# pylint: enable=invalid-name


class MeasuredScale(Enum):
    """
    Enumeration to help identify the scale of the measurements.
    """

    SECONDS = 0
    MILLISECONDS = 1
    MICROSECONDS = 2
    NANOSECONDS = 3
    COUNT = 4


class MeasuredDuration:
    """
    Class to encapsulate a duration along with the unit it is measured in.

    This is specifically targetted to durations between 1s and 1ns.
    """

    __unit_suffix = ["s", "ms", "\u00B5s", "ns", ""]

    def __init__(self, value, scale):
        self.__scale = scale
        self.__round_factor = (MeasuredScale.NANOSECONDS.value - scale.value) * 3
        self.__value = round(value, self.__round_factor)

    @property
    def value(self):
        """
        Value that was measured for the duration.
        """
        return self.__value

    @property
    def scale(self):
        """
        Scale associated with the measured duration.
        """
        return self.__scale

    def __str__(self):
        if self.scale == MeasuredScale.COUNT:
            ns_format = str(self.value)
        else:
            format_for_ns_precision = "{:." + str(self.__round_factor) + "f}"
            ns_format = format_for_ns_precision.format(self.__value)
        return f"{ns_format}{MeasuredDuration.__unit_suffix[self.__scale.value]}"

    def __repr__(self):
        return self.__str__()

    def get_delta(self, other):
        """
        Determine the delta between two durations that have the same scale.
        """
        assert self.scale == other.scale
        return MeasuredDuration(abs(self.value - other.value), self.__scale)

    @classmethod
    def percentile(cls, list_of_durations, percent):
        """
        Given a list of duration, compute the specified TP number for that percentage.
        """
        percentile_measurement = percentile(
            list_of_durations, percent, key=lambda x: x.value
        )
        return MeasuredDuration(percentile_measurement, list_of_durations[0].scale)


class ColumnFormatter:
    """
    Class to provide for very simple formating of the columns.
    """

    # pylint: disable=too-many-arguments
    def __init__(
        self,
        column_names,
        column_left_align,
        intercolumn_separator="  ",
        show_header=True,
        line_start_separator="",
    ):
        self.__column_names = column_names
        self.__column_widths = []
        for i in column_names:
            self.__column_widths.append(len(i))
        self.__data_columns = []

        assert len(column_left_align) == len(column_names)
        self.__column_left_align = column_left_align
        self.__intercolumn_separator = intercolumn_separator
        self.__show_header = show_header
        self.__line_start_separator = line_start_separator

    # pylint: enable=too-many-arguments

    def add_row(self, data):
        """
        Add a new row of data to be displayed.
        """
        assert len(data) == len(self.__column_names)
        new_row = []
        for column_index, column_data in enumerate(data):
            column_data_as_string = str(column_data)
            new_row.append(column_data_as_string)
            self.__column_widths[column_index] = max(
                self.__column_widths[column_index], len(column_data_as_string)
            )
        self.__data_columns.append(new_row)

    def __display_data(self, row_to_display):
        line_so_far = self.__line_start_separator
        for column_index, column_data in enumerate(row_to_display):
            if self.__column_left_align[column_index]:
                column_data = column_data.ljust(self.__column_widths[column_index])
            else:
                column_data = column_data.rjust(self.__column_widths[column_index])
            line_so_far += column_data + self.__intercolumn_separator
        print(line_so_far)

    def __generate_header_separator(self):
        separator_row = []
        for next_width in self.__column_widths:
            separator_row.append("".rjust(next_width, "-"))
        return separator_row

    def generate_output(self):
        """
        Generate output consistent with the data we have be asked to present.
        """
        if self.__show_header:
            self.__display_data(self.__column_names)
            self.__display_data(self.__generate_header_separator())
        for next_row in self.__data_columns:
            self.__display_data(next_row)


def __compute_scale_info(source_list_name, translate_scale):

    if source_list_name.endswith("-sec") or source_list_name.endswith("-seconds"):
        scale = MeasuredScale.SECONDS
    elif source_list_name.endswith("-millisec") or source_list_name.endswith("-ms"):
        scale = MeasuredScale.MILLISECONDS
    elif source_list_name.endswith("-microsec"):
        scale = MeasuredScale.MICROSECONDS
    elif source_list_name.endswith("-nanosec"):
        scale = MeasuredScale.NANOSECONDS
    else:
        scale = MeasuredScale.COUNT

    translation_factor = None
    compute_error = None
    if scale == MeasuredScale.COUNT:
        if translate_scale and translate_scale != MeasuredScale.COUNT:
            compute_error = (
                "Cannot translate from a COUNT measure to "
                + f"a {translate_scale.name} measure."
            )
    else:
        if translate_scale:
            scale_delta = translate_scale.value - scale.value
            if scale_delta > 0:
                translation_factor = pow(10, (scale_delta * 3))
            elif scale_delta < 0:
                translation_factor = pow(10, -(scale_delta * 3))

    return scale, translation_factor, compute_error


def __populate_measure(sample_data, duration_to_report):
    sample_data["value"] = duration_to_report.value
    sample_data["units"] = duration_to_report.scale.name.lower()


def __traverse_path(suite_data, source_list_name):
    try:
        if "." in source_list_name:
            sample_dictionary = suite_data
            split_source_name = source_list_name.split(".")
            for next_source_name in split_source_name:
                sample_dictionary = sample_dictionary[next_source_name]
        else:
            sample_dictionary = suite_data[source_list_name]
    except KeyError as this_error:
        print("suite_data:" + str(suite_data))
        raise this_error
    return sample_dictionary


# pylint: disable=too-many-arguments,too-many-locals
def __add_new_sample_set(
    formatter,
    suite_data,
    passed_test_indices,
    source_list_name,
    name_in_table,
    is_single_test_report,
    sample_data,
    translate_scale=None,
):
    """
    Look into the suite_data dictionary and interpret the specified sample set.
    """
    scale, translation_factor, compute_error = __compute_scale_info(
        source_list_name, translate_scale
    )
    if compute_error:
        print(f"Processing error: {compute_error}")
        return

    sample_dictionary = __traverse_path(suite_data, source_list_name)
    if is_single_test_report:
        data_value = sample_dictionary
        if translation_factor:
            data_value *= translation_factor
            scale = translate_scale
        duration_to_report = MeasuredDuration(data_value, scale)
        if formatter:
            row_values = [name_in_table, duration_to_report]
            formatter.add_row(row_values)
        else:
            __populate_measure(sample_data, duration_to_report)
    else:
        raw_measurements = []
        for index_in_data in passed_test_indices:
            scale = MeasuredScale.SECONDS
            data_value = None
            if 0 <= index_in_data < len(sample_dictionary):
                data_value = sample_dictionary[index_in_data]
            if not data_value:
                data_value = 0.0
            if translation_factor:
                data_value *= translation_factor
                scale = translate_scale

            raw_measurements.append(MeasuredDuration(data_value, scale))

        measurements = sorted(raw_measurements, key=lambda x: x.value)

        range_measure = measurements[-1].get_delta(measurements[0])
        tp50_measure = MeasuredDuration.percentile(measurements, 0.5)
        tp90_measure = MeasuredDuration.percentile(measurements, 0.9)

        if formatter:
            row_values = [
                name_in_table,
                measurements[0],
                measurements[-1],
                range_measure,
                tp50_measure,
                tp90_measure,
            ]
            formatter.add_row(row_values)
        else:
            sample_data["min"] = measurements[0].value
            sample_data["max"] = measurements[-1].value
            sample_data["range"] = range_measure.value
            sample_data["TP50"] = tp50_measure.value
            sample_data["TP90"] = tp90_measure.value
            sample_data["units"] = tp90_measure.scale.name.lower()


# pylint: enable=too-many-arguments,too-many-locals


def __output_suite_header(suite_data, formatted_header, suite_dictionary):
    source_data = suite_data["source"]

    file_name = source_data["file_name"]
    current_directory = os.path.abspath(os.getcwd())
    if file_name.startswith(current_directory):
        file_name = file_name[len(current_directory) :]

    if formatted_header:
        formatted_header.add_row(
            ["Source", f"{file_name}:{source_data['line_number']}"]
        )
    else:
        suite_dictionary["source"] = {}
        suite_dictionary["source"]["file_name"] = file_name
        suite_dictionary["source"]["line_number"] = source_data["line_number"]

    properties_data = suite_data["properties"]
    test_type = properties_data["test-type"]

    return_code_data = suite_data["per-test"]["return-code"]
    is_single_test_report = isinstance(return_code_data, int)
    return is_single_test_report, test_type


def __find_passed_tests(return_code_data, invoke_return_code_data):
    passed_test_indices = []
    test_index = 0
    for index, next_return_code in enumerate(return_code_data):

        if not next_return_code and not invoke_return_code_data[index]:
            passed_test_indices.append(test_index)
        test_index += 1
    return passed_test_indices


def __summarize_single_integration_test(
    return_code_data,
    invoke_return_code_data,
    suite_data,
    formatted_header,
    summary_data,
):
    return_code_data = max(return_code_data, invoke_return_code_data)

    test_duration = None
    if "measured-duration-sec" in suite_data["per-test"]:
        test_duration = suite_data["per-test"]["measured-duration-sec"]

    if formatted_header:
        formatted_header.add_row(
            ["Test Results", f"{__translate_return_code(return_code_data)}"]
        )
        if test_duration:
            formatted_header.add_row(
                [
                    "Test Duration (sec)",
                    f"{round(test_duration, DECIMALS_PLACES_IN_MILLISECONDS)}",
                ]
            )
    else:
        summary_data["test-results"] = __translate_return_code(return_code_data)
        if test_duration:
            summary_data["test-duration-seconds"] = round(
                test_duration, DECIMALS_PLACES_IN_MILLISECONDS
            )


# pylint: disable=too-many-branches
def __summarize_integration(
    suite_data, is_single_test_report, formatted_header, summary_data
):

    return_code_data = suite_data["per-test"]["return-code"]
    invoke_return_code_data = suite_data["per-test"]["invoke-return-code"]
    if is_single_test_report:
        __summarize_single_integration_test(
            return_code_data,
            invoke_return_code_data,
            suite_data,
            formatted_header,
            summary_data,
        )
    else:
        passed_test_indices = __find_passed_tests(
            return_code_data, invoke_return_code_data
        )
        if formatted_header:
            formatted_header.add_row(
                [
                    "Test Summary",
                    f"Success={len(passed_test_indices)}/{len(return_code_data)}",
                ]
            )
        else:
            summary_data["passed-tests"] = len(passed_test_indices)
            summary_data["total-tests"] = len(return_code_data)

        test_duration = None
        if "measured-duration-sec" in suite_data["per-test"]:
            test_duration = suite_data["per-test"]["measured-duration-sec"]
            translated_array = []
            for next_test in test_duration:
                translated_array.append(
                    f"{round(next_test, DECIMALS_PLACES_IN_MILLISECONDS)}"
                )
            test_duration = translated_array

        if len(passed_test_indices) != len(return_code_data):
            translated_array = []
            for index, next_test in enumerate(return_code_data):
                next_test = max(next_test, invoke_return_code_data[index])
                translated_array.append(__translate_return_code(next_test))

            if formatted_header:
                formatted_header.add_row(
                    [
                        "Test Results",
                        f"Success={translated_array}",
                    ]
                )
            else:
                summary_data["test-profiles"] = str(translated_array)
        if test_duration:
            if formatted_header:
                formatted_header.add_row(["Test Duration (sec)", f"{test_duration}"])
            else:
                summary_data["test-duration-seconds"] = test_duration


# pylint: enable=too-many-branches


def __summarize_performance_basics(
    suite_data, is_single_test_report, formatted_header, summary_data
):
    return_code_data = suite_data["per-test"]["return-code"]
    invoke_return_code_data = suite_data["per-test"]["invoke-return-code"]
    if is_single_test_report:
        return_code_data = max(return_code_data, invoke_return_code_data)
        passed_test_indices = None
        if formatted_header:
            formatted_header.add_row(
                ["Test Results", f"{__translate_return_code(return_code_data)}"]
            )
        else:
            summary_data["test-results"] = __translate_return_code(return_code_data)
    else:
        passed_test_indices = __find_passed_tests(
            return_code_data, invoke_return_code_data
        )
        if formatted_header:
            formatted_header.add_row(
                [
                    "Test Results",
                    f"Success={len(passed_test_indices)}/{len(return_code_data)}",
                ]
            )
        else:
            summary_data["passed-tests"] = len(passed_test_indices)
            summary_data["total-tests"] = len(return_code_data)

    return passed_test_indices


def __summarize_performance(
    suite_data, is_single_test_report, formatted_header, summary_data
):

    performance_legend_data = suite_data["performance-legend"]

    passed_test_indices = __summarize_performance_basics(
        suite_data, is_single_test_report, formatted_header, summary_data
    )

    formatted_columns = []
    formatted_data = None

    for next_display_value in performance_legend_data:

        if formatted_header:
            if not formatted_columns or next_display_value.startswith("."):
                if is_single_test_report:
                    formatted_data = ColumnFormatter(
                        ["Measure", "Value"],
                        [True, False],
                        line_start_separator="    ",
                    )
                else:
                    formatted_data = ColumnFormatter(
                        ["Measure", "Min", "Max", "Range", "TP50 (Med)", "TP90"],
                        [True, False, False, False, False, False],
                        line_start_separator="    ",
                    )
                formatted_columns.append(formatted_data)
        if next_display_value.startswith("."):
            continue

        sample_title = performance_legend_data[next_display_value].split(":")
        sample_scale = None
        if len(sample_title) > 1:
            sample_scale, _, _ = __compute_scale_info("-" + sample_title[1], None)

        sample_data = 0.0
        if summary_data:
            sample_data = {}
            summary_data[sample_title[0]] = sample_data

        __add_new_sample_set(
            formatted_data,
            suite_data,
            passed_test_indices,
            next_display_value,
            sample_title[0],
            is_single_test_report,
            sample_data,
            sample_scale,
        )
    return formatted_columns


def __generate_plain_output(data_dictionary):
    """
    Specifically generate output that is human readable.
    """

    for suite_title in data_dictionary:
        print(f"\n---\nTest: {suite_title}\n---")
        suite_data = data_dictionary[suite_title]

        formatted_header = ColumnFormatter(
            ["Item", "Value"],
            [True, True],
            show_header=False,
            line_start_separator="  ",
        )
        is_single_test_report, test_type = __output_suite_header(
            suite_data, formatted_header, None
        )
        formatted_header.add_row(["Test Type", f"{test_type}"])

        formatted_data_items = []
        if test_type == "performance":
            formatted_data_items = __summarize_performance(
                suite_data, is_single_test_report, formatted_header, None
            )
        else:
            __summarize_integration(
                suite_data, is_single_test_report, formatted_header, None
            )

        formatted_header.generate_output()
        for next_data_item in formatted_data_items:
            print(" ")
            next_data_item.generate_output()


def __generate_json_output(data_dictionary, workload_name):
    """
    Specifically generate output that is in json for parsing.
    """

    json_output = {}
    for suite_title in data_dictionary:
        suite_data = data_dictionary[suite_title]
        suite_dictionary = {}
        json_output[suite_title] = suite_dictionary

        is_single_test_report, test_type = __output_suite_header(
            suite_data, None, suite_dictionary
        )
        suite_dictionary["test-type"] = test_type
        suite_dictionary["workload-name"] = workload_name

        summary_data = {}
        suite_dictionary["summary"] = summary_data
        if test_type == "performance":
            __summarize_performance(
                suite_data, is_single_test_report, None, summary_data
            )
        else:
            __summarize_integration(
                suite_data, is_single_test_report, None, summary_data
            )
    print(json.dumps(json_output, indent=4))


def __process_script_action():
    """
    Process the posting of the message.
    """

    args = __process_command_line()
    input_file_name = "suite-results/summary.json"
    with open(input_file_name, encoding=__DEFAULT_FILE_ENCODING) as input_file:
        data_dictionary = json.load(input_file)

    if args.output_format == __JSON_FORMAT:
        __generate_json_output(data_dictionary, args.workload_name)
    else:
        __generate_plain_output(data_dictionary)


sys.exit(__process_script_action())

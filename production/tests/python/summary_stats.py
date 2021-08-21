#! /usr/bin/python3

"""
Script to translate a test's output.json file into a output.csv file.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import json
import sys
import os
import math
from enum import Enum

DECIMALS_PLACES_IN_MILLISECONDS = 3


def __translate_return_code(return_code):
    value = "unknown"
    if not return_code:
        value = "Success"
    elif return_code == 1:
        value = "SuiteFailure"
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


class MeasuredDuration:
    """
    Class to encapsulate a duration along with the unit it is measured in.

    This is specifically targetted to durations between 1s and 1ns.
    """

    __unit_suffix = ["s", "ms", "\u00B5s", "ns"]

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

    if source_list_name.endswith("-sec"):
        scale = MeasuredScale.SECONDS
    elif source_list_name.endswith("-millisec") or source_list_name.endswith("-ms"):
        scale = MeasuredScale.MILLISECONDS
    elif source_list_name.endswith("-microsec"):
        scale = MeasuredScale.MICROSECONDS
    else:
        assert source_list_name.endswith("-nanosec")
        scale = MeasuredScale.NANOSECONDS

    translation_factor = None
    if translate_scale:
        scale_delta = translate_scale.value - scale.value
        if scale_delta > 0:
            translation_factor = pow(10, (scale_delta * 3))
        elif scale_delta < 0:
            translation_factor = pow(10, -(scale_delta * 3))

    return scale, translation_factor


# pylint: disable=too-many-arguments,too-many-locals
def add_new_sample_set(
    formatter,
    suite_data,
    passed_test_indices,
    source_list_name,
    name_in_table,
    is_single_test_report,
    translate_scale=None,
):
    """
    Look into the suite_data dictionary and interpret the specified sample set.
    """
    scale, translation_factor = __compute_scale_info(source_list_name, translate_scale)

    if "." in source_list_name:
        sample_dictionary = suite_data
        split_source_name = source_list_name.split(".")
        for next_source_name in split_source_name:
            sample_dictionary = sample_dictionary[next_source_name]
    else:
        sample_dictionary = suite_data[source_list_name]

    if is_single_test_report:
        data_value = sample_dictionary
        if translation_factor:
            data_value *= translation_factor
            scale = translate_scale
        row_values = [name_in_table, MeasuredDuration(data_value, scale)]
    else:
        raw_measurements = []
        for index_in_data in passed_test_indices:
            data_value = sample_dictionary[index_in_data]
            if translation_factor:
                data_value *= translation_factor
                scale = translate_scale
            raw_measurements.append(MeasuredDuration(data_value, scale))

        measurements = sorted(raw_measurements, key=lambda x: x.value)

        range_measure = measurements[-1].get_delta(measurements[0])
        tp50_measure = MeasuredDuration.percentile(measurements, 0.5)
        tp90_measure = MeasuredDuration.percentile(measurements, 0.9)

        row_values = [
            name_in_table,
            measurements[0],
            measurements[-1],
            range_measure,
            tp50_measure,
            tp90_measure,
        ]
    formatter.add_row(row_values)


# pylint: enable=too-many-arguments,too-many-locals


def __output_suite_header(suite_data, formatted_header):
    source_data = suite_data["source"]

    file_name = source_data["file_name"]
    current_directory = os.path.abspath(os.getcwd())
    if file_name.startswith(current_directory):
        file_name = file_name[len(current_directory) :]
    formatted_header.add_row(["Source", f"{file_name}:{source_data['line_number']}"])

    properties_data = suite_data["properties"]
    test_type = properties_data["test-type"]

    iteration_data = suite_data["iterations"]
    is_single_test_report = isinstance(iteration_data, int)
    if test_type == "performance":

        if not is_single_test_report:
            iteration_data = iteration_data[0]
        formatted_header.add_row(["Steps/Iterations", f"{iteration_data}"])
    return is_single_test_report, test_type


def __find_passed_tests(return_code_data):
    passed_test_indices = []
    test_index = 0
    for next_return_code in return_code_data:
        if not next_return_code:
            passed_test_indices.append(test_index)
        test_index += 1
    return passed_test_indices


def __summarize_integration(suite_data, is_single_test_report, formatted_header):

    return_code_data = suite_data["return-code"]
    if is_single_test_report:
        passed_test_indices = None
        test_duration = suite_data["measured-duration-sec"]

        formatted_header.add_row(
            ["Test Results", f"{__translate_return_code(return_code_data)}"]
        )
        formatted_header.add_row(
            [
                "Test Duration (sec)",
                f"{round(test_duration, DECIMALS_PLACES_IN_MILLISECONDS)}",
            ]
        )
    else:
        passed_test_indices = __find_passed_tests(return_code_data)
        test_duration = suite_data["measured-duration-sec"]
        formatted_header.add_row(
            [
                "Test Summary",
                f"Success={len(passed_test_indices)}/{len(return_code_data)}",
            ]
        )
        translated_array = []
        for next_test in test_duration:
            translated_array.append(
                f"{round(next_test, DECIMALS_PLACES_IN_MILLISECONDS)}"
            )
        test_duration = translated_array

        if len(passed_test_indices) != len(return_code_data):
            translated_array = []
            for next_test in return_code_data:
                translated_array.append(__translate_return_code(next_test))

            formatted_header.add_row(
                [
                    "Test Results",
                    f"Success={translated_array}",
                ]
            )
        formatted_header.add_row(["Test Duration (sec)", f"{test_duration}"])


def __summarize_performance(suite_data, is_single_test_report, formatted_header):

    return_code_data = suite_data["return-code"]
    if is_single_test_report:
        passed_test_indices = None
        formatted_header.add_row(
            ["Test Results", f"{__translate_return_code(return_code_data)}"]
        )
        formatted_data = ColumnFormatter(
            ["Measure", "Value"],
            [True, False],
            line_start_separator="    ",
        )
        formatted_rules_data = ColumnFormatter(
            ["Measure", "Value"],
            [True, False],
            line_start_separator="    ",
        )
    else:
        passed_test_indices = __find_passed_tests(return_code_data)
        formatted_header.add_row(
            [
                "Test Results",
                f"Success={len(passed_test_indices)}/{len(return_code_data)}",
            ]
        )
        formatted_data = ColumnFormatter(
            ["Measure", "Min", "Max", "Range", "TP50 (Med)", "TP90"],
            [True, False, False, False, False, False],
            line_start_separator="    ",
        )
        formatted_rules_data = ColumnFormatter(
            ["Measure", "Min", "Max", "Range", "TP50 (Med)", "TP90"],
            [True, False, False, False, False, False],
            line_start_separator="    ",
        )

    add_new_sample_set(
        formatted_data,
        suite_data,
        passed_test_indices,
        "iteration-measured-duration-sec",
        "iter-total",
        is_single_test_report,
        MeasuredScale.MICROSECONDS,
    )
    add_new_sample_set(
        formatted_data,
        suite_data,
        passed_test_indices,
        "average-start-transaction-microsec",
        "start",
        is_single_test_report,
    )
    add_new_sample_set(
        formatted_data,
        suite_data,
        passed_test_indices,
        "average-inside-transaction-microsec",
        "inside",
        is_single_test_report,
    )
    add_new_sample_set(
        formatted_data,
        suite_data,
        passed_test_indices,
        "average-end-transaction-microsec",
        "commit",
        is_single_test_report,
    )
    add_new_sample_set(
        formatted_data,
        suite_data,
        passed_test_indices,
        "average-update-row-microsec",
        "update-row",
        is_single_test_report,
    )
    add_new_sample_set(
        formatted_data,
        suite_data,
        passed_test_indices,
        "average-wait-microsec",
        "wait",
        is_single_test_report,
    )
    add_new_sample_set(
        formatted_data,
        suite_data,
        passed_test_indices,
        "average-print-microsec",
        "print",
        is_single_test_report,
    )

    add_new_sample_set(
        formatted_rules_data,
        suite_data,
        passed_test_indices,
        "rules-engine-stats.calculations.avg-latency-ms",
        "avg-latency",
        is_single_test_report,
        MeasuredScale.MICROSECONDS,
    )
    add_new_sample_set(
        formatted_rules_data,
        suite_data,
        passed_test_indices,
        "rules-engine-stats.calculations.max-latency-ms",
        "max-latency",
        is_single_test_report,
        MeasuredScale.MICROSECONDS,
    )
    add_new_sample_set(
        formatted_rules_data,
        suite_data,
        passed_test_indices,
        "rules-engine-stats.calculations.avg-exec-ms",
        "avg-exec",
        is_single_test_report,
        MeasuredScale.MICROSECONDS,
    )
    add_new_sample_set(
        formatted_rules_data,
        suite_data,
        passed_test_indices,
        "rules-engine-stats.calculations.max-exec-ms",
        "max-exec",
        is_single_test_report,
        MeasuredScale.MICROSECONDS,
    )
    return formatted_data, formatted_rules_data


def process_script_action():
    """
    Process the posting of the message.
    """

    input_file_name = "suite-results/summary.json"
    with open(input_file_name) as input_file:
        data_dictionary = json.load(input_file)

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
            suite_data, formatted_header
        )
        formatted_header.add_row(["Test Type", f"{test_type}"])

        formatted_data, formatted_rules_data = (None, None)
        if test_type == "performance":
            formatted_data, formatted_rules_data = __summarize_performance(
                suite_data, is_single_test_report, formatted_header
            )
        else:
            __summarize_integration(suite_data, is_single_test_report, formatted_header)

        formatted_header.generate_output()
        if formatted_data:
            print(" ")
            formatted_data.generate_output()
        if formatted_rules_data:
            print(" ")
            formatted_rules_data.generate_output()


sys.exit(process_script_action())

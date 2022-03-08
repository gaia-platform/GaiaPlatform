#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide high level scenario tests for the Docker_Dev project.
"""

import io
from test.pytest_execute import InProcessExecution
from gdev.host import Host


class HostExecutor(InProcessExecution):
    """
    Class to provide a local instance of a InProcessExecution class.
    """

    def __init__(
        self,
        command_line_to_execute,
        is_failure_ok=False,
        single_line_mode=False,
        raw_mode=False,
    ):
        self.output_value = None
        self.__command_line_to_execute = command_line_to_execute
        self.__is_failure_ok = is_failure_ok
        self.__single_line_mode = single_line_mode
        self.__raw_mode = raw_mode

    def execute_main(self):
        """
        Invoke the proper instance of the main function.
        """
        if self.__raw_mode:
            self.output_value = Host.execute_sync(
                self.__command_line_to_execute, err_ok=self.__is_failure_ok
            )
        elif self.__single_line_mode:
            self.output_value = Host.execute_and_get_line_sync(
                self.__command_line_to_execute, err_ok=self.__is_failure_ok
            )
        else:
            self.output_value = Host.execute_and_get_lines_sync(
                self.__command_line_to_execute, err_ok=self.__is_failure_ok
            )

    def get_main_name(self):
        """
        Get the name of the entrypoint for "main".
        """
        return "main"


# pylint: disable=too-many-arguments
def __check_results(
    execution_result,
    host_executor,
    expected_return_code,
    expected_output,
    expected_error,
    expected_value_string,
):

    print(f"return_code: :{execution_result.return_code}")
    print(f"stdout:      :{execution_result.std_out.getvalue()}:")
    print(f"stderr:      :{execution_result.std_err.getvalue()}:")
    print(f"output_value::{host_executor.output_value}:")

    # Assert
    execution_result.assert_results(
        stdout=expected_output, stderr=expected_error, error_code=expected_return_code
    )
    if expected_value_string is not None:
        assert host_executor.output_value is not None
        if isinstance(host_executor.output_value, str):
            lined_output_value = host_executor.output_value
        else:
            lined_output_value = "\n".join(host_executor.output_value)
        compare_results = execution_result.assert_stream_contents(
            "value", io.StringIO(lined_output_value), expected_value_string
        )
        assert not compare_results, compare_results
    else:
        assert not host_executor.output_value, "Output value was expected to be None."


# pylint: enable=too-many-arguments


def test_execute_and_get_lines_sync_with_success():
    """
    Test to make sure a multi-line execution that succeeds reports back as
    intended.
    """

    # Arrange
    command_line_to_execute = "./gdev.sh --help"

    expected_return_code = 0
    expected_output = "\n"
    expected_error = ""
    expected_value_string = """usage: gdev [-h] {build,cfg,dockerfile,push,run} ...

positional arguments:
  {build,cfg,dockerfile,push,run}

optional arguments:
  -h, --help            show this help message and exit
"""

    # Act
    host_executor = HostExecutor(command_line_to_execute)
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_and_get_lines_sync_with_failure():
    """
    Test to make sure a multi-line execution with the `err_ok` flag unset returns
    failure information when the command is a failure.
    """

    # Arrange
    command_line_to_execute = "./gdev.sh --help_or"

    expected_return_code = 2
    expected_output = "\n"
    expected_error = """usage: gdev [-h] {build,cfg,dockerfile,push,run} ...
gdev: error: unrecognized arguments: --help_or

    """
    expected_value_string = None

    # Act
    host_executor = HostExecutor(command_line_to_execute)
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_and_get_lines_sync_with_ok_failure():
    """
    Test to make sure a multi-line execution with the `err_ok` flag set returns
    cleanly on failure with no output provided.
    """

    # Arrange
    command_line_to_execute = "./gdev.sh --help_or"
    is_command_failure_ok = True

    expected_return_code = 0
    expected_output = "\n"
    expected_error = ""
    expected_value_string = ""

    # Act
    host_executor = HostExecutor(command_line_to_execute, is_command_failure_ok)
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_and_get_line_sync_with_success():
    """
    Test to make sure a single-line execution that succeeds reports back as
    intended.
    """

    # Arrange
    command_line_to_execute = "./test/does_file_exist.sh README.md"

    expected_return_code = 0
    expected_output = "\n"
    expected_error = ""
    expected_value_string = """File does exist."""

    # Act
    host_executor = HostExecutor(command_line_to_execute, single_line_mode=True)
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_and_get_line_sync_with_failure():
    """
    Test to make sure a single line execution with the `err_ok` flag unset returns
    failure information when the command is a failure.
    """

    # Arrange
    command_line_to_execute = "./test/does_file_exist.sh not-an-existing-file.md"

    expected_return_code = 1
    expected_output = """File does not exist.

"""
    expected_error = "\n"
    expected_value_string = None

    # Act
    host_executor = HostExecutor(command_line_to_execute, single_line_mode=True)
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_and_get_line_sync_with_ok_failure():
    """
    Test to make sure a single line execution with the `err_ok` flag set returns
    cleanly on failure with no output provided.
    """

    # Arrange
    command_line_to_execute = "./test/does_file_exist.sh not-an-existing-file.md"
    is_command_failure_ok = True

    expected_return_code = 0
    expected_output = "\n"
    expected_error = ""
    expected_value_string = "File does not exist."

    # Act
    host_executor = HostExecutor(
        command_line_to_execute, is_command_failure_ok, single_line_mode=True
    )
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_and_get_line_sync_with_multiple_lines_failure():
    """
    Test to make sure a single line execution that returns mulitple lines provides
    the correct output.
    """

    # Arrange
    command_line_to_execute = "./test/echo_multiple_lines.sh"
    is_command_failure_ok = True

    expected_return_code = 1
    expected_output = (
        "Output of command `./test/echo_multiple_lines.sh` must only "
        + "contain one line, not:\nLine 1\nLine 2\n"
    )
    expected_error = ""
    expected_value_string = None

    # Act
    host_executor = HostExecutor(
        command_line_to_execute, is_command_failure_ok, single_line_mode=True
    )
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_sync_with_success():
    """
    Test to make sure a single-line execution that succeeds reports back as
    intended.
    """

    # Arrange
    command_line_to_execute = "./test/does_file_exist.sh README.md"

    expected_return_code = 0
    expected_output = "\n"
    expected_error = ""
    expected_value_string = None

    # Act
    host_executor = HostExecutor(command_line_to_execute, raw_mode=True)
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )


def test_execute_sync_with_failure():
    """
    Test to make sure a normal execution that succeeds reports back as
    intended.
    """

    # Arrange
    command_line_to_execute = "./test/does_file_exist.sh READMExx.md"

    expected_return_code = 1
    expected_output = "\n"
    expected_error = ""
    expected_value_string = None

    # Act
    host_executor = HostExecutor(command_line_to_execute, raw_mode=True)
    execution_result = host_executor.invoke_main()

    # Assert
    __check_results(
        execution_result,
        host_executor,
        expected_return_code,
        expected_output,
        expected_error,
        expected_value_string,
    )

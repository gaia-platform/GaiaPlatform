"""
Module to provide functionality to test scripts from within pytest.

This code copied from: https://github.com/jackdewinter/pyscan.
"""
import difflib
import io
import logging
import os
import sys
import traceback
from abc import ABC, abstractmethod

LOGGER = logging.getLogger(__name__)


class InProcessResult:
    """
    Class to provide for an encapsulation of the results of an execution.
    """

    def __init__(self, return_code, std_out, std_err):
        self.__return_code = return_code
        self.__std_out = std_out
        self.__std_err = std_err

    # pylint: disable=too-many-arguments
    @classmethod
    def compare_versus_expected(
        cls,
        stream_name,
        actual_stream,
        expected_text,
        additional_text=None,
        log_extra=None,
    ):
        """
        Do a thorough comparison of the actual stream against the expected text.
        """

        if additional_text:
            assert actual_stream.getvalue().strip().startswith(expected_text.strip()), (
                f"Block\n---\n{expected_text}\n---\nwas not found at the start of"
                + "\n---\n{actual_stream.getvalue()}\nExtra:{log_extra}"
            )

            for next_text_block in additional_text:
                was_found = next_text_block.strip() in actual_stream.getvalue().strip()
                diff = difflib.ndiff(
                    next_text_block.strip().splitlines(),
                    actual_stream.getvalue().strip().splitlines(),
                )

                diff_values = "\n".join(list(diff))
                print(diff_values, file=sys.stderr)
                if not was_found:
                    assert (
                        False
                    ), f"Block\n---\n{next_text_block}\n---\nwas not found in\n---\n{actual_stream.getvalue()}"
        elif actual_stream.getvalue().strip() != expected_text.strip():
            diff = difflib.ndiff(
                expected_text.splitlines(), actual_stream.getvalue().splitlines()
            )

            diff_values = "\n".join(list(diff)) + "\n---\n"

            LOGGER.warning(
                "actual>>%s",
                cls.__make_value_visible(actual_stream.getvalue()),
            )
            print(f"WARN>actual>>{cls.__make_value_visible(actual_stream.getvalue())}")
            LOGGER.warning("expect>>%s", cls.__make_value_visible(expected_text))
            print(f"WARN>expect>>{cls.__make_value_visible(expected_text)}")
            if log_extra:
                print(f"log_extra:{log_extra}")
            assert False, f"{stream_name} not as expected:\n{diff_values}"

    # pylint: enable=too-many-arguments

    # pylint: disable=unused-private-member
    @classmethod
    def __make_value_visible(cls, string_to_modify):
        return string_to_modify.replace("\n", "\\n").replace("\t", "\\t")

    # pylint: enable=unused-private-member

    @property
    def return_code(self):
        """
        Return code provided after execution.
        """
        return self.__return_code

    @property
    def std_out(self):
        """
        Standard output collected during execution.
        """
        return self.__std_out

    # pylint: disable=too-many-arguments
    def assert_results(
        self,
        stdout=None,
        stderr=None,
        error_code=0,
        additional_error=None,
        alternate_stdout=None,
    ):
        """
        Assert the results are as expected in the "assert" phase.
        """

        try:
            if stdout:
                if alternate_stdout:
                    try:
                        self.compare_versus_expected(
                            "Stdout",
                            self.__std_out,
                            stdout,
                            log_extra=self.__std_err.getvalue(),
                        )
                    except AssertionError:
                        self.compare_versus_expected(
                            "Stdout",
                            self.__std_out,
                            alternate_stdout,
                            log_extra=self.__std_err.getvalue(),
                        )
                else:
                    self.compare_versus_expected(
                        "Stdout",
                        self.__std_out,
                        stdout,
                        log_extra=self.__std_err.getvalue(),
                    )
            else:
                assert_text = (
                    f"Expected stdout to be empty, not: {self.__std_out.getvalue()}"
                )
                if self.__std_err.getvalue():
                    assert_text += f"\nStdErr was:{self.__std_err.getvalue()}"
                assert not self.__std_out.getvalue(), assert_text

            if stderr:
                self.compare_versus_expected(
                    "Stderr", self.__std_err, stderr, additional_error
                )
            else:
                assert (
                    not self.__std_err.getvalue()
                ), f"Expected stderr to be empty, not: {self.__std_err.getvalue()}"

            assert (
                self.__return_code == error_code
            ), f"Actual error code ({self.__return_code}) and expected error code ({error_code}) differ."

        finally:
            self.__std_out.close()
            self.__std_err.close()

    # pylint: enable=too-many-arguments

    def assert_stream_contents(
        self, stream_name, actual_stream, expected_stream, additional_error=None
    ):
        """
        Assert that the contents of the given stream are as expected.
        """

        result = None
        try:
            if expected_stream:
                self.compare_versus_expected(
                    stream_name, actual_stream, expected_stream, additional_error
                )
            else:
                assert not actual_stream.getvalue(), (
                    "Expected "
                    + stream_name
                    + " to be empty. Not:\n---\n"
                    + actual_stream.getvalue()
                    + "\n---\n"
                )
        except AssertionError as ex:
            result = ex
        finally:
            actual_stream.close()
        return result

    @classmethod
    def assert_resultant_file(cls, file_path, expected_contents):
        """
        Assert the contents of a given file against it's expected contents.
        """

        split_expected_contents = expected_contents.split("\n")
        with open(file_path, "r", encoding="utf-8") as infile:
            split_actual_contents = infile.readlines()
        for line_index, line_content in enumerate(split_actual_contents):
            if line_content[-1] == "\n":
                split_actual_contents[line_index] = line_content[:-1]

        are_different = len(split_expected_contents) != len(split_actual_contents)
        if not are_different:
            index = 0
            while index < len(split_expected_contents):
                are_different = (
                    split_expected_contents[index] != split_actual_contents[index]
                )
                if are_different:
                    break
                index += 1

        if are_different:
            diff = difflib.ndiff(split_actual_contents, split_expected_contents)
            diff_values = "\n".join(list(diff))
            assert (
                False
            ), f"Actual and expected contents of '{file_path}' are not equal:\n---\n{diff_values}\n---\n"


# pylint: disable=too-few-public-methods
class SystemState:
    """
    Class to provide an encapsulation of the system state so that we can restore
    it later.
    """

    def __init__(self):
        """
        Initializes a new instance of the SystemState class.
        """

        self.saved_stdout = sys.stdout
        self.saved_stderr = sys.stderr
        self.saved_cwd = os.getcwd()
        self.saved_env = os.environ
        self.saved_argv = sys.argv

    def restore(self):
        """
        Restore the system state variables to what they were before.
        """

        os.chdir(self.saved_cwd)
        os.environ = self.saved_env
        sys.argv = self.saved_argv
        sys.stdout = self.saved_stdout
        sys.stderr = self.saved_stderr


# pylint: enable=too-few-public-methods


class InProcessExecution(ABC):
    """
    Handle the in-process execution of the script's mainline.
    """

    @abstractmethod
    def execute_main(self):
        """
        Provides the code to execute the mainline.  Should be simple like:
        MyObjectClass().main()
        """

    @abstractmethod
    def get_main_name(self):
        """
        Provides the main name to associate with the mainline.  Gets set as
        the first argument to the program.
        """

    @classmethod
    def handle_system_exit(cls, exit_exception, std_error):
        """
        Handle the processing of an "early" exit as a result of our execution.
        """
        returncode = exit_exception.code
        if isinstance(returncode, str):
            std_error.write("f{exit_exception}\n")
            returncode = 1
        elif returncode is None:
            returncode = 0
        return returncode

    @classmethod
    def handle_normal_exception(cls):
        """
        Handle the processing of a normal exception as a result of our execution.
        """
        try:
            exception_type, exception_value, trace_back = sys.exc_info()
            traceback.print_exception(
                exception_type, exception_value, trace_back.tb_next
            )
        finally:
            del trace_back
        return 1

    # pylint: disable=broad-except
    def invoke_main(self, arguments=None, cwd=None):
        """
        Invoke the mainline so that we can capture results.
        """

        saved_state = SystemState()

        std_output = io.StringIO()
        std_error = io.StringIO()
        sys.stdout = std_output
        sys.stderr = std_error

        sys.argv = arguments.copy() if arguments else []
        sys.argv.insert(0, self.get_main_name())

        if cwd:
            os.chdir(cwd)

        try:
            returncode = 0
            self.execute_main()
        except SystemExit as this_exception:
            returncode = self.handle_system_exit(this_exception, std_error)
        except Exception:
            returncode = self.handle_normal_exception()
        finally:
            saved_state.restore()

        return InProcessResult(returncode, std_output, std_error)

    # pylint: enable=broad-except
import os
from test.pytest_execute import InProcessExecution
from gdev.__main__ import main
from gdev.main import DockerDev

"""
This code copied from: https://github.com/jackdewinter/pyscan/blob/test/test_scenarios.py
"""

class MainlineExecutor(InProcessExecution):
    """
    Class to provide for a local instance of a InProcessExecution class.
    """

    def __init__(self, use_module=True, use_main=False):
        super().__init__()

        self.__use_main = use_main
        self.__entry_point = "__main__.py" if use_module else "main.py"

    def execute_main(self):
        if self.__use_main:
            main()
        else:
            DockerDev().main()

    def get_main_name(self):
        return self.__entry_point

            # new_process = subprocess.Popen(
            #     command_line.split(" "), cwd=self.__script_directory
            # )

            # poll_result = next_process.poll()
            # if poll_result is not None:
#             process.wait()

def test_show_help():
    """
    Make sure that we can show help about the various things to do.
    """

    # Arrange
    executor = MainlineExecutor()
    suppplied_arguments = ["--help"]

    expected_output = """\
main.py 0.5.0
"""
    expected_error = ""
    expected_return_code = 0

    # Act
    execute_results = executor.invoke_main(arguments=suppplied_arguments, cwd=None)

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

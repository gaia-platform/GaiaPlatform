from test.pytest_execute import InProcessExecution, InProcessResult
from gdev.__main__ import main
from gdev.main import DockerDev
import os
import io
import subprocess

"""
This code mostly copied from: https://github.com/jackdewinter/pyscan/blob/test/test_scenarios.py
"""

class SubprocessExecutor():
    def __init__(self, script_path=None):
        self.__script_path = script_path if script_path else os.path.realpath(os.path.join(determine_repository_base_directory(), "dev_tools", "docker_dev", "gdev.sh"))

    def invoke_main(self, arguments=None, cwd=None):

        arguments = arguments.copy()
        arguments.insert(0, self.__script_path)
        cwd = cwd if cwd else determine_repository_production_directory()

        new_process = subprocess.Popen(arguments, cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        process_return_code = new_process.wait()

        process_stdout_output = ""
        for line in io.TextIOWrapper(new_process.stdout, encoding="utf-8"):
            process_stdout_output += line

        process_stderr_output = ""
        for line in io.TextIOWrapper(new_process.stderr, encoding="utf-8"):
            process_stderr_output += line

        return InProcessResult(process_return_code, io.StringIO(process_stdout_output), io.StringIO(process_stderr_output))

class MainlineExecutor(InProcessExecution):
    """
    Class to provide a local instance of a InProcessExecution class.
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

def determine_repository_base_directory():
    script_directory = os.path.dirname(os.path.realpath(__file__))
    base_directory = os.path.realpath(os.path.join(script_directory, "..", "..", ".."))
    return base_directory

def determine_repository_production_directory():
    production_directory = os.path.realpath(os.path.join(determine_repository_base_directory(), "production"))
    return production_directory

def determine_old_script_behavior(gdev_arguments):
    original_gdev_script_path = os.path.realpath(os.path.join(determine_repository_base_directory(), "dev_tools", "gdev", "gdev.sh"))
    executor = SubprocessExecutor(original_gdev_script_path)

    arguments_to_use = [original_gdev_script_path]
    arguments_to_use.extend(gdev_arguments)

    new_process = subprocess.Popen(arguments_to_use, cwd=determine_repository_production_directory(),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    process_return_code = new_process.wait()

    process_stdout_output = ""
    for line in io.TextIOWrapper(new_process.stdout, encoding="utf-8"):
        process_stdout_output += line

    process_stderr_output = ""
    for line in io.TextIOWrapper(new_process.stderr, encoding="utf-8"):
        process_stderr_output += line

    return process_return_code, process_stdout_output, process_stderr_output

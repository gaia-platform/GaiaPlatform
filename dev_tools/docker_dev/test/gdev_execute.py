#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to contain executors with which to execute the application within tests.

This code copied from: https://github.com/jackdewinter/pyscan/blob/test/test_scenarios.py
with small changes.
Any changes made to this code above the base code are copyright by Gaia Platform LLC.
"""

import os
import io
import subprocess
from test.pytest_execute import InProcessExecution, InProcessResult
from gdev.__main__ import main
from gdev.main import DockerDev

# pylint: disable=too-few-public-methods
class SubprocessExecutor():
    """
    Class to use a subprocess to execute the tests.
    """
    def __init__(self, script_path=None):
        self.__script_path = script_path \
            if script_path else \
                os.path.realpath(os.path.join(determine_repository_base_directory(), \
                    "dev_tools", "docker_dev", "gdev.sh"))

    def invoke_main(self, arguments=None, cwd=None):
        """
        Invoke the main function with the proper parameters.
        """

        arguments = arguments.copy()
        arguments.insert(0, self.__script_path)
        cwd = cwd if cwd else determine_repository_production_directory()

        with subprocess.Popen(arguments, cwd=cwd,stdout=subprocess.PIPE,
            stderr=subprocess.PIPE) as new_process:

            process_return_code = new_process.wait()

            process_stdout_output = "".join(io.TextIOWrapper(new_process.stdout, encoding="utf-8"))
            process_stderr_output = "".join(io.TextIOWrapper(new_process.stderr, encoding="utf-8"))

            return InProcessResult(process_return_code, io.StringIO(process_stdout_output), \
                io.StringIO(process_stderr_output))
# pylint: enable=too-few-public-methods

class MainlineExecutor(InProcessExecution):
    """
    Class to provide a local instance of a InProcessExecution class.
    """

    def __init__(self, use_module=True, use_main=False):
        super().__init__()

        self.__use_main = use_main
        self.__entry_point = "__main__.py" if use_module else "main.py"

    def execute_main(self):
        """
        Invoke the proper instance of the main function.
        """
        if self.__use_main:
            main()
        else:
            DockerDev().main()

    def get_main_name(self):
        """
        Get the name of the entrypoint for "main".
        """
        return self.__entry_point

def determine_repository_base_directory():
    """
    Determine the location of the repository's base directory.
    """
    script_directory = os.path.dirname(os.path.realpath(__file__))
    base_directory = os.path.realpath(os.path.join(script_directory, "..", "..", ".."))
    return base_directory

def determine_repository_production_directory():
    """
    Determine the location of the repository's production directory.
    """
    production_directory = os.path.realpath(
        os.path.join(determine_repository_base_directory(), "production"))
    return production_directory

def determine_old_script_behavior(gdev_arguments):
    """
    Determine the behavior of the old gdev script.
    """
    original_gdev_script_path = os.path.realpath(
        os.path.join(determine_repository_base_directory(), "dev_tools", "gdev", "gdev.sh"))
    #executor = SubprocessExecutor(original_gdev_script_path)

    arguments_to_use = [original_gdev_script_path]
    arguments_to_use.extend(gdev_arguments)

    with subprocess.Popen(arguments_to_use, \
        cwd=determine_repository_production_directory(),stdout=subprocess.PIPE,
        stderr=subprocess.PIPE) as new_process:

        process_return_code = new_process.wait()

        process_stdout_output = "".join(io.TextIOWrapper(new_process.stdout, encoding="utf-8"))
        process_stderr_output = "".join(io.TextIOWrapper(new_process.stderr, encoding="utf-8"))
        return process_return_code, process_stdout_output, process_stderr_output

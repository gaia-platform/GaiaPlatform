#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to handle communications with the host system.
"""

from logging import getLogger
import sys
from typing import Optional, Sequence
import subprocess
import io

from gdev.third_party.atools import memoize


log = getLogger(__name__)

class Host:
    """
    Class to handle communications with the host system.
    """

    __is_drydock_enabled = False

    """
    Class to handle communications with the host system.
    """
    @staticmethod
    def set_drydock(value):
        """
        Set the drydock value to enable reporting of commands instead of executing them.
        """
        Host.__is_drydock_enabled = value

    @staticmethod
    def is_drydock_enabled():
        """
        Determine if drydock mode is enabled.
        """
        return Host.__is_drydock_enabled

    @staticmethod
    @memoize
    def __execute_sync(command: str, err_ok: bool,
        capture_output: bool)-> Optional[Sequence[str]]:

        log.debug('execute_sync err_ok = %s capture_output = %s command = %s',
            err_ok, capture_output, command)
        with subprocess.Popen(
            command.replace("  ", " ").split(" "),
            stdout=subprocess.PIPE if capture_output else None,
            stderr=subprocess.PIPE if capture_output else None) as child_process:

            process_return_code = child_process.wait()
            log.debug('execute: return_code= %s, command= %s', process_return_code, command)

            if process_return_code == 0 or err_ok:
                if child_process.stdout is None:
                    return tuple()
                process_stdout_output = \
                    "".join(io.TextIOWrapper(child_process.stdout, encoding="utf-8"))
                return tuple(process_stdout_output.strip().splitlines())
            if child_process.stdout is not None:
                process_stdout_output = \
                    "".join(io.TextIOWrapper(child_process.stdout, encoding="utf-8"))
                print(process_stdout_output, file=sys.stdout)
            if child_process.stderr is not None:
                process_stderr_output = \
                    "".join(io.TextIOWrapper(child_process.stderr, encoding="utf-8"))
                print(process_stderr_output, file=sys.stderr)
            sys.exit(process_return_code)

    @staticmethod
    def execute_sync(command: str, *, err_ok: bool = False) -> None:
        """
        Execute the specified command string without capturing any of the output.
        """
        if Host.is_drydock_enabled():
            print(f"[execute:{command}]")
        else:
            Host.__execute_sync(capture_output=False, command=command, err_ok=err_ok)

    @staticmethod
    def execute_and_get_lines_sync(command: str, *, err_ok: bool = False) -> Sequence[str]:
        """
        Execute the specified command string and capture the output.
        """
        return Host.__execute_sync(capture_output=True, command=command, err_ok=err_ok)

    @staticmethod
    def execute_and_get_line_sync(command: str, *, err_ok: bool = False) -> str:
        """
        Execute the specified command string and capture the single line of output.
        """
        if Host.is_drydock_enabled() and command.startswith("docker image inspect"):
            lines = ["<no value>"]
        else:
            lines = Host.__execute_sync(capture_output=True, command=command, err_ok=err_ok)
        assert len(lines) == 1, f'Must contain one line: {lines = }'
        return lines[0]

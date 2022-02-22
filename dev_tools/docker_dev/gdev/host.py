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
    def __execute_sync(command: str, err_ok: bool, capture_output: bool) -> Optional[Sequence[str]]:

        log.debug(f'execute_sync {err_ok = } {capture_output = } {command = }')
        child_process = subprocess.Popen(
            command.replace("  ", " ").split(" "),
            stdout=subprocess.PIPE if capture_output else None,
            stderr=subprocess.PIPE if capture_output else None)

        process_return_code = child_process.wait()
        log.debug(f'execute: return_code= {process_return_code}, command= {command}')

        if process_return_code == 0 or err_ok:
            if child_process.stdout is None:
                return tuple()
            else:
                process_stdout_output = ""
                for line in io.TextIOWrapper(child_process.stdout, encoding="utf-8"):
                    process_stdout_output += line
                return tuple(process_stdout_output.strip().splitlines())
        else:
            if child_process.stdout is not None:
                process_stdout_output = ""
                for line in io.TextIOWrapper(child_process.stdout, encoding="utf-8"):
                    process_stdout_output += line
                print(process_stdout_output, file=sys.stdout)
            if child_process.stderr is not None:
                process_stderr_output = ""
                for line in io.TextIOWrapper(child_process.stderr, encoding="utf-8"):
                    process_stderr_output += line
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

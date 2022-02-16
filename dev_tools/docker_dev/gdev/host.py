from asyncio.subprocess import create_subprocess_exec, create_subprocess_shell, PIPE, Process
from dataclasses import dataclass
from logging import getLogger
import os
import sys
from typing import Optional, Sequence

from gdev.third_party.atools import memoize


log = getLogger(__name__)


@dataclass(frozen=True)
class Host:

    __is_drydock_enabled = False

    """
    Class to handle communications with the host system.
    """
    @staticmethod
    def set_drydock(value):
        Host.__is_drydock_enabled = value

    @staticmethod
    def is_drydock_enabled():
        return Host.__is_drydock_enabled

    @staticmethod
    async def __finish_process(err_ok: bool, process: Process, command:str) -> Sequence[str]:

        stdout, stderr = await process.communicate()
        log.debug(f'execute: return_code= {process.returncode}, command= {command}')
        if process.returncode == 0 or err_ok:
            if stdout is None:
                return tuple()
            else:
                return tuple(stdout.decode().strip().splitlines())
        else:
            if stdout is not None:
                print(stdout.decode(), file=sys.stdout)
            if stderr is not None:
                print(stderr.decode(), file=sys.stderr)
            sys.exit(process.returncode)

    @staticmethod
    @memoize
    async def __execute(command: str, err_ok: bool, capture_output: bool) -> Optional[Sequence[str]]:
        log.debug(f'execute {err_ok = } {capture_output = } {command = }')
        process = await create_subprocess_exec(
            *command.split(),
            stdout=PIPE if capture_output else None,
            stderr=PIPE if capture_output else None,
            env=os.environ,
        )
        return await Host.__finish_process(err_ok=err_ok, process=process, command=command)

    @staticmethod
    async def execute(command: str, *, err_ok: bool = False) -> None:
        """
        Execute the specified command string without capturing any of the output. 
        """
        if Host.is_drydock_enabled:
            print(f"[execute:{command}]")
        else:
            await Host.__execute(capture_output=False, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_and_get_lines(command: str, *, err_ok: bool = False) -> Sequence[str]:
        """
        Execute the specified command string and capture the output. 
        """
        return await Host.__execute(capture_output=True, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_and_get_line(command: str, *, err_ok: bool = False) -> str:
        """
        Execute the specified command string and capture the single line of output. 
        """
        if Host.is_drydock_enabled and command.startswith("docker image inspect"):
            lines = ["<no value>"]
        else:
            lines = await Host.__execute(capture_output=True, command=command, err_ok=err_ok)
        assert len(lines) == 1, f'Must contain one line: {lines = }'
        return lines[0]

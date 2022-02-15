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

    __xx = False

    """
    Class to handle communications with the host system.
    """
    @staticmethod
    def xx(value):
        Host.__xx = value

    @staticmethod
    async def __finish_process(err_ok: bool, process: Process) -> Sequence[str]:

        stdout, stderr = await process.communicate()
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
        return await Host.__finish_process(err_ok=err_ok, process=process)

    @staticmethod
    async def execute(command: str, *, err_ok: bool = False) -> None:
        """
        Execute the specified command string without capturing any of the output. 
        """
        print(">>>>>" + str(Host.__xx))
        if Host.__xx:
            print(f"[execute:{command}]")
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
        lines = await Host.__execute(capture_output=True, command=command, err_ok=err_ok)
        assert len(lines) == 1, f'Must contain one line: {lines = }'
        return lines[0]

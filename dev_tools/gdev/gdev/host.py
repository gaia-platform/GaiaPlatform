from asyncio.subprocess import (
    create_subprocess_exec, create_subprocess_shell, PIPE, Process, STDOUT
)
from dataclasses import dataclass
from logging import getLogger
import os
import sys
from typing import Iterable, Optional, Sequence

from gdev.third_party.atools import memoize


log = getLogger(__name__)


@dataclass(frozen=True)
class Host:

    class SubprocessException(Exception):
        pass

    @staticmethod
    async def _finish_process(
            capture_output: bool, err_ok: bool, process: Process
    ) -> Optional[Iterable[str]]:

        await process.wait()

        if (process.returncode != 0) and (not err_ok):
            if process.stdout is not None:
                print((await process.stdout.read()).decode())
            if process.stderr is not None:
                print((await process.stderr.read()).decode())
            sys.exit(process.returncode)

        if capture_output:
            return tuple(((await process.communicate())[0]).decode().strip().splitlines())
        else:
            return None

    @staticmethod
    @memoize
    async def _execute(command: str, err_ok: bool, capture_output: bool) -> Optional[Sequence[str]]:
        log.debug(f'execute {err_ok = } {capture_output = } {command = }')
        process = await create_subprocess_exec(
            *command.split(),
            stdout=PIPE if capture_output else None,
            stderr=STDOUT if capture_output else None,
            env=os.environ,
        )
        return await Host._finish_process(
            capture_output=capture_output,
            err_ok=err_ok,
            process=process,
        )

    @staticmethod
    async def execute(command: str, *, err_ok: bool = False) -> None:
        await Host._execute(capture_output=False, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_and_get_lines(command: str, *, err_ok: bool = False) -> Iterable[str]:
        return await Host._execute(capture_output=True, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_and_get_line(command: str, *, err_ok: bool = False) -> str:
        lines = await Host._execute(capture_output=True, command=command, err_ok=err_ok)
        assert len(lines) == 1, f'Must contain one line: {lines = }'

        return lines[0]

    @staticmethod
    @memoize
    async def _execute_shell(
            command: str, *, capture_output: bool, err_ok: bool
    ) -> Optional[Sequence[str]]:
        log.debug(f'execute_shell {err_ok = } {capture_output = } {command = }')
        process = await create_subprocess_shell(
            command,
            stdout=PIPE if capture_output else None,
            stderr=STDOUT if capture_output else None,
            env=os.environ,
        )

        return await Host._finish_process(
            process=process, err_ok=err_ok, capture_output=capture_output
        )

    @staticmethod
    async def execute_shell(command: str, *, err_ok: bool = False) -> Iterable[str]:
        return await Host._execute_shell(capture_output=False, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_shell_and_get_lines(command: str, *, err_ok: bool = False) -> Iterable[str]:
        return await Host._execute_shell(capture_output=True, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_shell_and_get_line(command: str, *, err_ok: bool = False) -> str:
        lines = await Host._execute_shell(capture_output=True, command=command, err_ok=err_ok)
        assert len(lines) == 1, f'Must contain one line: {lines = }'

        return lines[0]

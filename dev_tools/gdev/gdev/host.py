from asyncio.subprocess import (
    create_subprocess_exec, create_subprocess_shell, PIPE, Process, STDOUT
)
from dataclasses import dataclass
from logging import getLogger
import os
from textwrap import indent
from typing import Iterable, Optional, Sequence

from gdev.custom.pathlib import Path
from gdev.third_party.atools import memoize


log = getLogger(__name__)


@dataclass(frozen=True)
class Host:

    class SubprocessException(Exception):
        pass

    @staticmethod
    async def _finish_process(
            capture_output: bool, command: str, err_ok: bool, process: Process
    ) -> Optional[Iterable[str]]:

        await process.wait()

        if (process.returncode != 0) and (not err_ok):
            raise Host.SubprocessException(
                f'Command "{command}" exited code: {process.returncode}'
            )

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
            command=command,
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
            process=process, command=command, err_ok=err_ok, capture_output=capture_output
        )

    @staticmethod
    async def execute_shell(command: str, *, err_ok: bool = False) -> Iterable[str]:
        return await Host._execute_shell(capture_output=True, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_shell_and_get_lines(command: str, *, err_ok: bool = False) -> Iterable[str]:
        return await Host._execute_shell(capture_output=True, command=command, err_ok=err_ok)

    @staticmethod
    async def execute_shell_and_get_line(command: str, *, err_ok: bool = False) -> str:
        lines = await Host._execute_shell(capture_output=True, command=command, err_ok=err_ok)
        assert len(lines) == 1, f'Must contain one line: {lines = }'

        return lines[0]

    @staticmethod
    def read_bytes(path: Path) -> bytes:
        data = path.read_bytes()

        log.debug(f'Read from {path}, data:\n{indent(data.decode(), "    ")}')

        return data

    @staticmethod
    def read_text(path: Path) -> str:
        data = path.read_text()

        log.debug(f'Read from {path}, data: \n{indent(data, "    ")}')

        return data

    @staticmethod
    def write_bytes(*, path: Path, data: bytes) -> None:
        log.debug(f'Write to {path}, bytes:\n{indent(data.decode(), "    ")}')
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    @staticmethod
    def write_text(*, path: Path, data: str) -> None:
        log.debug(f'Writing to {path}, text: \n{indent(data, "    ")}')
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(data)

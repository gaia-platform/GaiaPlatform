from abc import ABC, abstractmethod
import re
from typing import Iterable, Pattern

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize


class GenAbcCfg(Dependency, ABC):

    @abstractmethod
    async def get_section_name(self) -> str:
        raise NotImplementedError

    @memoize
    async def get_path(self) -> Path:
        path = Path.repo() / self.options.target / 'gdev.cfg'

        self.log.debug(f'{path = }')

        return path

    @memoize
    async def get_begin_pattern(self) -> Pattern:
        begin_pattern = re.compile(fr'^.*\[{await self.get_section_name()}].*$')

        self.log.debug(f'{begin_pattern = }')

        return begin_pattern

    @memoize
    async def get_end_pattern(self) -> Pattern:
        end_pattern = re.compile(fr'^.*\[.+].*$')

        self.log.debug(f'{end_pattern = }')

        return end_pattern

    @classmethod
    @memoize
    async def _get_text(cls, path: Path) -> str:
        if path.is_file():
            text = Host.read_text(path)
        else:
            text = ''

        return text

    @memoize
    async def get_lines(self) -> Iterable[str]:
        lines = (await self._get_text(await self.get_path())).splitlines()

        ilines = iter(lines)
        lines = []
        begin_pattern = await self.get_begin_pattern()
        for iline in ilines:
            if begin_pattern.match(iline):
                break

        end_pattern = await self.get_end_pattern()
        for iline in ilines:
            if end_pattern.match(iline):
                break
            lines.append(iline)

        ilines = iter(lines)
        lines = []
        for iline in ilines:
            line_parts = [iline]
            while line_parts[-1].endswith('\\'):
                line_parts.append(next(ilines))
            lines.append('\\\n    '.join(line_parts))

        lines = [line for line in lines if (line and not line.startswith('#'))]

        lines = [
            eval(
                f'f""" {line} """',
                {'build_dir': Path.build, 'source_dir': Path.source}
            )[1:-1]
            for line in lines
        ]

        lines = tuple(lines)

        self.log.debug(f'{lines = }')

        return lines

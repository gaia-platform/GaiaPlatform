from abc import ABC
from inspect import getfile
import re
from typing import Iterable, Pattern

from gdev.custom.pathlib import Path
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize


class GenAbcCfg(Dependency, ABC):
    """Parse gdev.cfg for build rules."""

    @property
    @memoize
    def path(self) -> Path:
        path = Path.repo() / self.options.target / 'gdev.cfg'

        self.log.debug(f'{path = }')

        return path

    @property
    def section_name(self) -> str:
        return Path(getfile(type(self))).parent.name

    @memoize
    async def get_begin_pattern(self) -> Pattern:
        begin_pattern = re.compile(fr'^\[{self.section_name}]\w*$')

        self.log.debug(f'{begin_pattern = }')

        return begin_pattern

    @memoize
    async def get_end_pattern(self) -> Pattern:
        end_pattern = re.compile(fr'^\[.+]\w*$')

        self.log.debug(f'{end_pattern = }')

        return end_pattern

    @memoize
    async def get_lines(self) -> Iterable[str]:
        lines = (await self._get_raw_text(self.path)).splitlines()

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
            lines.append('\n    '.join(line_parts))

        lines = [line for line in lines if (line and not line.startswith('#'))]

        lines = [
            eval(f'fr""" {line} """', {'build_dir': Path.build, 'source_dir': Path.source})[1:-1]
            for line in lines
        ]

        lines = tuple(lines)

        self.log.debug(f'{lines = }')

        return lines

    @staticmethod
    @memoize
    async def _get_raw_text(path: Path) -> str:
        if not path.is_file():
            raise Dependency.Abort(f'File "<repo_root>/{path.context()}" must exist.')

        text = path.read_text()

        return text

    @memoize
    async def cli_entrypoint(self) -> None:
        print(f'[{self.section_name}]\n' + '\n'.join(await self.get_lines()))

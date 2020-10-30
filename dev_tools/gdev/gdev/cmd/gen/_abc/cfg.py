from abc import ABC
from inspect import getfile
import re
from typing import FrozenSet, Iterable, Pattern

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
        return Path(getfile(type(self))).parent.name.strip('_')

    @memoize
    async def get_begin_pattern(self) -> Pattern:
        begin_pattern = re.compile(fr'^\[{self.section_name}]$')

        self.log.debug(f'{begin_pattern = }')

        return begin_pattern

    @memoize
    async def get_end_pattern(self) -> Pattern:
        end_pattern = re.compile(fr'^(# .*|)\[.+]$')

        self.log.debug(f'{end_pattern = }')

        return end_pattern

    @staticmethod
    @memoize
    async def get_lines(cfg_enables: FrozenSet[str], path: Path) -> Iterable[str]:
        return tuple([
            eval(
                f'fr""" {line} """',
                {
                    'build_dir': Path.build,
                    'enable_if': lambda enable:
                        '' if enable in cfg_enables
                        else f'# enable by setting "{enable}": ',
                    'enable_if_not': lambda enable:
                        '' if enable not in cfg_enables
                        else f'# enable by not setting "{enable}": ',
                    'enable_if_any': lambda *enables:
                        '' if set(enables) & cfg_enables
                        else f'# enable by setting any of "{set(enables)}": ',
                    'enable_if_not_any': lambda *enables:
                        '' if not (set(enables) & cfg_enables)
                        else f'# enable by not setting any of "{set(enables)}": ',
                    'enable_if_all': lambda *enables:
                        '' if set(enables) in cfg_enables
                        else f'# enable by setting all of "{set(enables)}": ',
                    'enable_if_not_all': lambda *enables:
                        '' if not (set(enables) in cfg_enables)
                        else f'# enable by not setting all of "{set(enables)}": ',
                    'source_dir': Path.source
                }
            )[1:-1]
            for line in (await GenAbcCfg._get_raw_text(path)).splitlines()
        ])

    @staticmethod
    @memoize
    async def _get_raw_text(path: Path) -> str:
        if not path.is_file():
            raise Dependency.Abort(f'File "<repo_root>/{path.context()}" must exist.')

        text = path.read_text()

        return text

    @memoize
    async def get_section_lines(self) -> Iterable[str]:
        lines = await self.get_lines(cfg_enables=self.options.cfg_enables, path=self.path)

        ilines = iter(lines)
        section_lines = []
        begin_pattern = await self.get_begin_pattern()
        for iline in ilines:
            if begin_pattern.match(iline):
                break

        end_pattern = await self.get_end_pattern()
        for iline in ilines:
            if end_pattern.match(iline):
                break
            section_lines.append(iline)

        ilines = iter(section_lines)
        section_lines = []
        for iline in ilines:
            line_parts = [iline]
            while line_parts[-1].endswith('\\'):
                if not (next_line := next(ilines)).strip().startswith('#'):
                    line_parts.append(next_line)
            section_lines.append('\n    '.join(line_parts))

        section_lines = [
            section_line
            for section_line in section_lines
            if (section_line and not section_line.startswith('#'))
        ]

        section_lines = tuple(section_lines)

        self.log.debug(f'{section_lines = }')

        return section_lines

    @memoize
    async def cli_entrypoint(self) -> None:
        print(f'[{self.section_name}]\n' + '\n'.join(await self.get_section_lines()))

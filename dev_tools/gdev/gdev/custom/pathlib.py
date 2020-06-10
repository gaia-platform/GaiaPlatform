from __future__ import annotations

from logging import getLogger
from pathlib import PosixPath
from subprocess import check_output
from typing import Union


log = getLogger(__name__)


class Path(PosixPath):
    _repo: Path = None

    @classmethod
    def build(cls, path: Union[Path, str]) -> Path:
        return Path('/build') / path

    def context(self) -> Path:
        if self.is_absolute():
            return self.relative_to(self.repo())
        else:
            return self

    def image_build(self) -> Path:
        return Path.build(self.context())

    def image_source(self) -> Path:
        return Path.source(self.context())

    @classmethod
    def mixin(cls) -> Path:
        return cls.repo() / 'dev_tools' / 'gdev' / 'mixin'

    @classmethod
    def repo(cls) -> Path:
        if cls._repo is None:
            repo = Path(
                check_output(
                    f'git rev-parse --show-toplevel'.split(),
                    cwd=f'{Path(__file__).parent}'
                ).decode().strip()
            )

            log.debug(f'{repo = }')

            cls._repo = repo

        return cls._repo

    @classmethod
    def source(cls, path: Union[Path, str]) -> Path:
        return Path('/source') / path

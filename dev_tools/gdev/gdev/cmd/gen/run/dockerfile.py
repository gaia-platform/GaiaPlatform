from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenRunCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenRunDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenRunCfg:
        return GenRunCfg(self.options)

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..pre_run.dockerfile import GenPreRunDockerfile

        input_dockerfiles = tuple([GenPreRunDockerfile(self.options)])

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles

    @memoize
    async def get_run_section(self) -> str:
        if lines := await self.cfg.get_lines():
            run_section = (
                    'RUN '
                    + ' \\\n    && '.join(lines)
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

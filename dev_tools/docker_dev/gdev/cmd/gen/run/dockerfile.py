from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenRunCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenRunDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenRunCfg:
        return GenRunCfg(self.options)

    @memoize
    async def get_env_section(self) -> str:
        from ..pre_run.dockerfile import GenPreRunDockerfile

        env_section = await GenPreRunDockerfile(self.options).get_env_section()

        self.log.debug(f'{env_section = }')

        return env_section

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..pre_run.dockerfile import GenPreRunDockerfile

        input_dockerfiles = tuple([GenPreRunDockerfile(self.options)])

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles

    @memoize
    async def get_run_section(self) -> str:
        if section_lines := await self.cfg.get_section_lines():
            run_section = (
                    'RUN '
                    + ' \\\n    && '.join(section_lines)
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

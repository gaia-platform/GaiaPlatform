from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenPreRunCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenPreRunDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenPreRunCfg:
        return GenPreRunCfg(self.options)

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..apt.dockerfile import GenAptDockerfile
        from ..env.dockerfile import GenEnvDockerfile
        from ..gaia.dockerfile import GenGaiaDockerfile
        from ..git.dockerfile import GenGitDockerfile
        from ..pip.dockerfile import GenPipDockerfile
        from ..web.dockerfile import GenWebDockerfile

        input_dockerfiles = tuple([
            GenAptDockerfile(self.options),
            GenEnvDockerfile(self.options),
            GenGaiaDockerfile(self.options),
            GenGitDockerfile(self.options),
            GenPipDockerfile(self.options),
            GenWebDockerfile(self.options),
        ])

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

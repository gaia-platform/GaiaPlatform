from typing import Iterable

from gdev.third_party.atools import memoize
from ._abc.dockerfile import GenAbcDockerfile
from .cfg import GenCfg
from .run.cfg import GenRunCfg
from .run.dockerfile import GenRunDockerfile


class GenDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenCfg:
        return GenCfg(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcDockerfile]:
        direct_inputs = tuple([GenRunDockerfile(self.options)])

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs

    @memoize
    async def get_run_section(self) -> str:
        if lines := await GenRunCfg(self.options).get_lines():
            run_statement = (
                    'RUN '
                    + ' \\\n    && '.join(lines)
            )
        else:
            run_statement = ''

        self.log.debug(f'{run_statement = }')

        return run_statement

from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenRunCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenRunDockerfile(GenAbcDockerfile):
    """
    Class to execute the steps necessary to generate a dockerfile based
    on the configuration.
    """

    @property
    def cfg(self) -> GenRunCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenRunCfg(self.options)

    @memoize
    async def get_env_section(self) -> str:
        """
        Return text for the ENV section of the final build stage.
        """
        from ..pre_run.dockerfile import GenPreRunDockerfile

        env_section = await GenPreRunDockerfile(self.options).get_env_section()

        self.log.debug(f'{env_section = }')

        return env_section

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        """
        Return dockerfiles that describe build stages that come directly before this one.
        """
        from ..pre_run.dockerfile import GenPreRunDockerfile

        input_dockerfiles = tuple([GenPreRunDockerfile(self.options)])

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles

    @memoize
    async def get_run_section(self) -> str:
        """
        Return text for the RUN line of the final build stage.
        """
        if section_lines := await self.cfg.get_section_lines():
            run_section = (
                    'RUN '
                    + ' \\\n    && '.join(section_lines)
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

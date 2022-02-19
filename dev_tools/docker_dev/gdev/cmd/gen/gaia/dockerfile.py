from dataclasses import replace
from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenGaiaCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenGaiaDockerfile(GenAbcDockerfile):
    """
    Class to handle the processing of the 'gaia' section of the 'gdev.cfg' file.
    """

    @property
    def cfg(self) -> GenGaiaCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenGaiaCfg(self.options)

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        """
        Return dockerfiles that describe build stages that come directly before this one.
        """

        from ..run.dockerfile import GenRunDockerfile

        input_dockerfiles = []
        for section_line in await self.cfg.get_section_lines():
            input_dockerfiles.append(GenRunDockerfile(replace(self.options, target=section_line)))
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles

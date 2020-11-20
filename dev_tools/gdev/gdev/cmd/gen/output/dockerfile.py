from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenOutputCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenOutputDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenOutputCfg:
        return GenOutputCfg(self.options)

    @memoize
    async def get_copy_section(self) -> str:
        copy_section_parts = []
        for input_dockerfile in await self.get_input_dockerfiles():
            if lines := await self.cfg.get_section_lines():
                for line in lines:
                    copy_section_parts.append(
                        f'COPY --from={await input_dockerfile.get_name()} {line} {line}'
                    )
            elif copy_section := await super()._get_copy_section():
                copy_section_parts.append(copy_section)
        copy_section = '\n'.join(copy_section_parts)

        self.log.debug(f'{copy_section = }')

        return copy_section

    @memoize
    async def get_env_section(self) -> str:
        from ..pre_run.dockerfile import GenPreRunDockerfile

        env_section = await GenPreRunDockerfile(self.options).get_env_section()

        self.log.debug(f'{env_section = }')

        return env_section

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..run.dockerfile import GenRunDockerfile

        input_dockerfiles = tuple([GenRunDockerfile(self.options)])

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles

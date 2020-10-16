from gdev.third_party.atools import memoize
from .cfg import GenPipCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenPipDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenPipCfg:
        return GenPipCfg(self.options)

    @memoize
    async def get_from_section(self) -> str:
        from_section = f'FROM pip_base AS {await self.get_name()}'

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    async def get_run_section(self) -> str:
        if section_lines := await self.cfg.get_section_lines():
            run_section = (
                'RUN python3 -m pip install '
                + ' \\\n        '.join(section_lines)
                + ' \\\n    && apt-get remove --autoremove -y python3-pip'
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

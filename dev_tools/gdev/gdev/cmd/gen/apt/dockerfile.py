from gdev.third_party.atools import memoize
from .cfg import GenAptCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenAptDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenAptCfg:
        return GenAptCfg(self.options)

    @memoize
    async def get_from_section(self) -> str:
        from_section = f'FROM apt_base as {await self.get_name()}'

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    async def get_run_section(self) -> str:
        if lines := await self.cfg.get_lines():
            run_section = (
                    'RUN DEBIAN_FRONTEND=noninteractive apt-get install -y'
                    + ' \\\n        '
                    + ' \\\n        '.join(lines)
                    + ' \\\n    && apt-get clean'
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

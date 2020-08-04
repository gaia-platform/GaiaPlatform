from gdev.third_party.atools import memoize
from .cfg import GenWebCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenWebDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenWebCfg:
        return GenWebCfg(self.options)

    @memoize
    async def get_from_section(self) -> str:
        from_section = f'FROM web_base AS {await self.get_name()}'

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    async def get_run_section(self) -> str:
        if lines := await self.cfg.get_lines():
            run_statement = (
                f'RUN wget '
                + ' \\\n        '.join(lines)
                + ' \\\n    && apt-get remove --autoremove -y wget'
            )
        else:
            run_statement = ''

        self.log.debug(f'{run_statement = }')

        return run_statement

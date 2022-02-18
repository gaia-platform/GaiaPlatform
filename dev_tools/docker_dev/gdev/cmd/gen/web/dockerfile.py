from gdev.third_party.atools import memoize
from .cfg import GenWebCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenWebDockerfile(GenAbcDockerfile):
    """
    Class to generate the WEB section of the dockerfile.
    """

    @property
    def cfg(self) -> GenWebCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenWebCfg(self.options)

    @memoize
    async def get_from_section(self) -> str:
        """
        Return text for the FROM line of the final build stage.
        """
        from_section = f'FROM web_base AS {await self.get_name()}'

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    async def get_run_section(self) -> str:
        """
        Return text for the RUN line of the final build stage.
        """

        if section_lines := await self.cfg.get_section_lines():
            run_statement = (
                f'RUN wget '
                + ' \\\n        '.join(section_lines)
                + ' \\\n    && apt-get remove --autoremove -y wget'
            )
        else:
            run_statement = ''

        self.log.debug(f'{run_statement = }')

        return run_statement

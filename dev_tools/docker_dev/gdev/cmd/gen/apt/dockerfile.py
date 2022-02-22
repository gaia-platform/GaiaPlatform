"""
Module to generate the APT section of the dockerfile.
"""
from gdev.third_party.atools import memoize
from .cfg import GenAptCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenAptDockerfile(GenAbcDockerfile):
    """
    Class to generate the APT section of the dockerfile.
    """

    @property
    def cfg(self) -> GenAptCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenAptCfg(self.options)

    @memoize
    def get_from_section(self) -> str:
        """
        Return text for the FROM line of the final build stage.
        """
        from_section = f'FROM apt_base AS {self.get_name()}'

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    def get_run_section(self) -> str:
        """
        Return text for the RUN line of the final build stage.
        """
        if section_lines := self.cfg.get_section_lines():
            run_section = (
                    'RUN apt-get update'
                    + ' \\\n    && DEBIAN_FRONTEND=noninteractive apt-get install -y'
                    + ' \\\n        '
                    + ' \\\n        '.join(section_lines)
                    + ' \\\n    && apt-get clean'
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

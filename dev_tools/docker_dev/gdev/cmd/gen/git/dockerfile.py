"""
Module to generate the GIT section of the dockerfile.
"""

from gdev.third_party.atools import memoize
from .cfg import GenGitCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenGitDockerfile(GenAbcDockerfile):
    """
    Class to generate the GIT section of the dockerfile.
    """

    @property
    def cfg(self) -> GenGitCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenGitCfg(self.options)

    @memoize
    def get_from_section(self) -> str:
        """
        Return text for the FROM line of the final build stage.
        """
        from_section = f'FROM git_base AS {self.get_name()}'

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    def get_run_section(self) -> str:
        """
        Return text for the RUN line of the final build stage.
        """
        if section_lines := self.cfg.get_section_lines():
            run_section = (
                    'RUN '
                    + ' \\\n    && '.join(
                        [f'git clone --depth 1 {section_line}' for section_line in section_lines]
                    )
                    + ' \\\n    && rm -rf */.git'
                    + ' \\\n    && apt-get remove --autoremove -y git'
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

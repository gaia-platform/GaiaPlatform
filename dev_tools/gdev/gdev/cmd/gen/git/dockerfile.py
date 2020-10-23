from gdev.third_party.atools import memoize
from .cfg import GenGitCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenGitDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenGitCfg:
        return GenGitCfg(self.options)

    @memoize
    async def get_from_section(self) -> str:
        from_section = f'FROM git_base AS {await self.get_name()}'

        self.log.debug(f'{from_section = }')

        return from_section

    @memoize
    async def get_run_section(self) -> str:
        if section_lines := await self.cfg.get_section_lines():
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

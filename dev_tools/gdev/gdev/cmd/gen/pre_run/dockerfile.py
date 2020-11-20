from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenPreRunCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenPreRunDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenPreRunCfg:
        return GenPreRunCfg(self.options)

    @memoize
    async def get_env_section(self) -> str:
        """Return text for the ENV section of the final build stage."""
        from ..env.dockerfile import GenEnvDockerfile

        seen_env_dockerfiles = set()

        async def inner(env_dockerfile: GenEnvDockerfile) -> Iterable[str]:
            env_section_parts = []
            if env_dockerfile not in seen_env_dockerfiles:
                seen_env_dockerfiles.add(env_dockerfile)
                for input_env_dockerfile in await env_dockerfile.get_input_dockerfiles():
                    env_section_parts += await inner(GenEnvDockerfile(input_env_dockerfile.options))
                if section_lines := await env_dockerfile.cfg.get_section_lines():
                    env_section_parts.append(f'# {env_dockerfile}')
                    for line in section_lines:
                        env_section_parts.append(f'ENV {line}')
            return env_section_parts

        env_section = '\n'.join(await inner(GenEnvDockerfile(self.options)))

        self.log.debug(f'{env_section = }')

        return env_section

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..apt.dockerfile import GenAptDockerfile
        from ..env.dockerfile import GenEnvDockerfile
        from ..gaia.dockerfile import GenGaiaDockerfile
        from ..git.dockerfile import GenGitDockerfile
        from ..pip.dockerfile import GenPipDockerfile
        from ..web.dockerfile import GenWebDockerfile

        input_dockerfiles = tuple([
            GenAptDockerfile(self.options),
            GenEnvDockerfile(self.options),
            GenGitDockerfile(self.options),
            GenPipDockerfile(self.options),
            GenWebDockerfile(self.options),
            GenGaiaDockerfile(self.options),
        ])

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles

    @memoize
    async def get_run_section(self) -> str:
        if section_lines := await self.cfg.get_section_lines():
            run_section = (
                    'RUN '
                    + ' \\\n    && '.join(section_lines)
            )
        else:
            run_section = ''

        self.log.debug(f'{run_section = }')

        return run_section

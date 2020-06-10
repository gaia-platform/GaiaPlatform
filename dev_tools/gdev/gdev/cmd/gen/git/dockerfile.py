from gdev.third_party.atools import memoize
from .cfg import GenGitCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenGitDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenGitCfg:
        return GenGitCfg(self.options)

    @memoize
    async def get_run_section(self) -> str:
        if lines := await self.cfg.get_lines():
            run_statement = (
                    'RUN apt-get update'
                    + ' \\\n    && DEBIAN_FRONTEND=noninteractive apt-get install -y git'
                    + ' \\\n    && '
                    + ' \\\n    && '.join([f'git clone {line}' for line in lines])
                    + ' \\\n    && apt-get remove --autoremove -y git'
                    + ' \\\n    && apt-get clean'
            )
        else:
            run_statement = ''

        self.log.debug(f'{run_statement = }')

        return run_statement

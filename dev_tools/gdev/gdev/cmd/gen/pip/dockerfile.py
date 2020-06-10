from gdev.third_party.atools import memoize
from .cfg import GenPipCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenPipDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenPipCfg:
        return GenPipCfg(self.options)

    @memoize
    async def get_run_section(self) -> str:
        if lines := await self.cfg.get_lines():
            run_statement = (
                'RUN apt-get update'
                + ' \\\n    && DEBIAN_FRONTEND=noninteractive apt-get install -y python3-pip'
                + ' \\\n    && python3 -m pip install'
                + ' \\\n        '
                + ' \\\n        '.join(lines)
                + ' \\\n    && apt-get remove --autoremove -y python3-pip'
                + ' \\\n    && apt-get clean'
            )
        else:
            run_statement = ''

        self.log.debug(f'{run_statement = }')

        return run_statement

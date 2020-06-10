from gdev.third_party.atools import memoize
from .cfg import GenWebCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenWebDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenWebCfg:
        return GenWebCfg(self.options)

    @memoize
    async def get_run_section(self) -> str:
        if lines := await self.cfg.get_lines():
            run_statement = (
                'RUN apt-get update'
                + ' \\\n    && DEBIAN_FRONTEND=noninteractive apt-get install -y wget'
                + ' \\\n    && wget --no-verbose'
                + ' \\\n        '
                + ' \\\n        '.join(lines)
                + ' \\\n    && apt-get remove --autoremove -y wget'
                + ' \\\n    && apt-get clean'
            )
        else:
            run_statement = ''

        self.log.debug(f'{run_statement = }')

        return run_statement

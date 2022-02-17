from gdev.third_party.atools import memoize
from .gen.run.cfg import GenRunCfg


class Cfg(GenRunCfg):

    @memoize
    async def cli_entrypoint(self) -> None:
        print(
            '\n'.join(
                await self.get_lines(
                    cfg_enables=self.options.cfg_enables,
                    path=self.path
                )
            )
        )

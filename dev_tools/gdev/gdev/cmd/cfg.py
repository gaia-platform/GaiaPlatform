from gdev.third_party.atools import memoize
from .gen.run.cfg import GenRunCfg


class Cfg(GenRunCfg):

    @memoize
    async def cli_entrypoint(self) -> None:
        print(self.path.read_text())

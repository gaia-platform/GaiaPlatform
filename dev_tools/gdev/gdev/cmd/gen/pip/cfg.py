from gdev.third_party.atools import memoize
from .._abc.cfg import GenAbcCfg


class GenPipCfg(GenAbcCfg):

    @memoize
    async def get_section_name(self) -> str:
        section_name = 'pip'

        self.log.debug(f'{section_name = }')

        return section_name

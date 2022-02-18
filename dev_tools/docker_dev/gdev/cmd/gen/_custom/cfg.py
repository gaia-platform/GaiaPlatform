"""
Module to provide for a subclass of the GenAbcCfg class for the Apt section where MIXINs are used.
"""
from typing import Iterable

from gdev.custom.pathlib import Path
from gdev.third_party.atools import memoize
from .._abc.cfg import GenAbcCfg


class GenCustomCfg(GenAbcCfg):
    """
    Class to provide for a subclass of the GenAbcCfg class for the Apt section where MIXINs are used.
    """

    @memoize
    def get_lines(self) -> Iterable[str]:
        """
        Get the various lines for the section, all derived from the needed mixins.
        """
        lines = tuple([f'{Path.mixin().context() / mixin}' for mixin in sorted(self.options.mixins)])

        self.log.info(f'{lines = }')

        return lines

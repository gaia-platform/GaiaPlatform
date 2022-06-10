#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide a subclass of the GenAbcCfg class for the Apt section where MIXINs are used.
"""

from typing import Iterable

from gdev.custom.gaia_path import GaiaPath
from gdev.third_party.atools import memoize
from gdev.sections._abc.cfg import GenAbcCfg


class GenCustomCfg(GenAbcCfg):
    """
    Class to provide for a subclass of the GenAbcCfg class for the Apt section where MIXINs
    are used.
    """

    @memoize
    def get_mixin_lines(self) -> Iterable[str]:
        """
        Get the various lines for the section, all derived from the needed mixins.
        """
        lines = tuple(
            (
                f"{GaiaPath.mixin().context() / mixin}"
                for mixin in sorted(self.options.mixins)
            )
        )

        self.log.info("lines = %s", lines)

        return lines

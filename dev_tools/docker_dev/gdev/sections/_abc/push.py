#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide for the necessary actions to perform a push of the image.
"""

from abc import abstractmethod
from gdev.sections._abc.gdev_action import GdevAction
from gdev.host import Host
from gdev.third_party.atools import memoize
from gdev.sections._abc.build import GenAbcBuild


class GenAbcPush(GdevAction):
    """
    Class to provide for the necessary actions to perform a push of the image.
    """

    @property
    @abstractmethod
    def build(self) -> GenAbcBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        raise NotImplementedError

    @memoize
    def main(self) -> None:
        """
        Main action undertaken by this class.
        """
        self.build.main()

        tag = self.build.get_tag()
        Host.execute_sync(f"docker tag {tag} {self.options.registry}/{tag}")
        Host.execute_sync(f"docker push {self.options.registry}/{tag}")

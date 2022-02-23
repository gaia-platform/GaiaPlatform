#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the necessary actions to perform a push of the image.
"""
from abc import ABC, abstractmethod

from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from gdev.cmd.gen._abc.build import GenAbcBuild


class GenAbcPush(Dependency, ABC):
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
        Host.execute_sync(f'docker tag {tag} {self.options.registry}/{tag}')
        Host.execute_sync(f'docker push {self.options.registry}/{tag}')

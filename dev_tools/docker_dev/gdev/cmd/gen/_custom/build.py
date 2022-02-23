#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile where MIXINs are used.
"""

from dataclasses import dataclass
from typing import Mapping

from gdev.third_party.atools import memoize
from gdev.cmd.gen._custom.dockerfile import GenCustomDockerfile
from gdev.cmd.gen._abc.build import GenAbcBuild


@dataclass(frozen=True, repr=False)
class GenCustomBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile where MIXINs are used.
    """

    base_build: GenAbcBuild

    @property
    def dockerfile(self) -> GenCustomDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenCustomDockerfile(self.options, self.base_build.dockerfile)

    @memoize
    def _get_wanted_label_value_by_name(self) -> Mapping[str, str]:
        """
        Get the hash of the image with the label values that are called for by the configuration.
        """
        return {
            **super()._get_wanted_label_value_by_name(),
            "Mixins": f"{sorted(self.options.mixins)}".replace(" ", ""),
        }

    @memoize
    def _get_actual_label_value_by_name(self) -> Mapping[str, str]:
        """
        Get the hash of an image with the actual label values that are called
        for by the configuration.
        """
        return {
            **super()._get_actual_label_value_by_name(),
            "Mixins": super()._get_actual_label_value("Mixins"),
        }

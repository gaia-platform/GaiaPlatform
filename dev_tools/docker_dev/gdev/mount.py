#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to represent a mount point between the docker container and the host system.
"""

from dataclasses import dataclass
from gdev.custom.gaia_path import GaiaPath


@dataclass(frozen=True)
class Mount:
    """
    Class to represent a mount point between the docker container and the host system.
    """

    container_path: GaiaPath
    host_path: GaiaPath

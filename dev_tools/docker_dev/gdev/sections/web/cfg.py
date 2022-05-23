#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide a subclass of the GenAbcCfg class for the Web section.
"""

from gdev.sections._abc.cfg import GenAbcCfg


class GenWebCfg(GenAbcCfg):
    """
    Class to provide a subclass of the GenAbcCfg class for the Web section.

    Note that this class is empty, but kept to maintain a subcommand structure
    that is equivalent to the other subcommands, such as build.
    """

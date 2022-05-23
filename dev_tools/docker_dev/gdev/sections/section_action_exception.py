#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide for a simple action to be used to denote expected section action failures.
"""


class SectionActionException(Exception):
    """
    Class to provide for a simple action to be used to denote expected section action failures.
    """

    def __str__(self) -> str:
        return f"Abort: {super().__str__()}"

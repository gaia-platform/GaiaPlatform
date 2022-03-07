#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for a simple action to be used to denote expected section action failures.
"""


class SectionActionException(Exception):
    """
    Class to provide for a simple action to be used to denote expected section action failures.
    """

    def __str__(self) -> str:
        return f"Abort: {super().__str__()}"

"""
Module to provide a subclass of the GenAbcCfg class for the Gaia section.
"""
from gdev.third_party.atools import memoize
from .._abc.cfg import GenAbcCfg


class GenGaiaCfg(GenAbcCfg):
    """
    Class to provide a subclass of the GenAbcCfg class for the Gaia section.

    Note that this class is empty, but kept to maintain a subcommand structure
    that is equivalent to the other subcommands, such as build.
    """
    pass

"""
Module to provide for a subclass of the GenAbcCfg class for the Pip section.
"""
from .._abc.cfg import GenAbcCfg


class GenPipCfg(GenAbcCfg):
    """
    Class to provide for a subclass of the GenAbcCfg class for the Pip section.

    Note that this class is empty, but kept to maintain a subcommand structure
    that is equivalent to the other subcommands, such as build.
    """
    pass

"""
Module to provide for a subclass of the GenAbcCfg class for the Git section.
"""
from .._abc.cfg import GenAbcCfg


class GenGitCfg(GenAbcCfg):
    """
    Class to provide for a subclass of the GenAbcCfg class for the Git section.

    Note that this class is empty, but kept to maintain a subcommand structure
    that is equivalent to the other subcommands, such as build.
    """
    pass

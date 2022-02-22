"""
Module to satisfy the push requirements for the PRERUN section.
"""
from .build import GenPreRunBuild
from .._abc.push import GenAbcPush


class GenPreRunPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the PRERUN section.
    """

    @property
    def build(self) -> GenPreRunBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenPreRunBuild(self.options)

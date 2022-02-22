"""
Module to satisfy the push requirements for the RUN section.
"""
from .build import GenRunBuild
from .._abc.push import GenAbcPush


class GenRunPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the RUN section.
    """

    @property
    def build(self) -> GenRunBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenRunBuild(self.options)

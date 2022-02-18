"""
Module to satisfy the run requirements to for the PRERUN section.
"""
from .build import GenPreRunBuild
from .._abc.run import GenAbcRun


class GenPreRunRun(GenAbcRun):
    """
    Class to satisfy the run requirements to for the PRERUN section.
    """

    @property
    def build(self) -> GenPreRunBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenPreRunBuild(self.options)

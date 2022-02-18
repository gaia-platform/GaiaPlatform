"""
Module to satisfy the run requirements to for the RUN section.
"""
from .build import GenRunBuild
from .._abc.run import GenAbcRun


class GenRunRun(GenAbcRun):
    """
    Class to satisfy the run requirements to for the RUN section.
    """

    @property
    def build(self) -> GenRunBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenRunBuild(self.options)

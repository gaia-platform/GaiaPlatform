"""
Module to satisfy the run requirements to for the ENV section.
"""
from .build import GenEnvBuild
from .._abc.run import GenAbcRun


class GenEnvRun(GenAbcRun):
    """
    Class to satisfy the run requirements to for the ENV section.
    """

    @property
    def build(self) -> GenEnvBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenEnvBuild(self.options)

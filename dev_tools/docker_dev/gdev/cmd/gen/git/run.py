"""
Module to satisfy the run requirements to for the GIT section.
"""
from .._abc.build import GenAbcBuild
from .._abc.run import GenAbcRun


class GenGitRun(GenAbcRun):
    """
    Class to satisfy the run requirements to for the GIT section.
    """

    @property
    def build(self) -> GenAbcBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        from .build import GenGitBuild
        return GenGitBuild(self.options)

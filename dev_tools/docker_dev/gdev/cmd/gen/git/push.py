"""
Module to satisfy the push requirements for the GIT section.
"""
from .build import GenGitBuild
from .._abc.push import GenAbcPush


class GenGitPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the GIT section.
    """

    @property
    def build(self) -> GenGitBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenGitBuild(self.options)

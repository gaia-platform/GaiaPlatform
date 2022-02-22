"""
Module to satisfy the build requirements to generate the dockerfile for the GIT section.
"""
from .dockerfile import GenGitDockerfile
from .._abc.build import GenAbcBuild


class GenGitBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the GIT section.
    """

    @property
    def dockerfile(self) -> GenGitDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenGitDockerfile(self.options)

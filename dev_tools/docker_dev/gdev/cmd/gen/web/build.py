"""
Module to satisfy the build requirements to generate the dockerfile for the WEB section.
"""
from .dockerfile import GenWebDockerfile
from .._abc.build import GenAbcBuild


class GenWebBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the WEB section.
    """

    @property
    def dockerfile(self) -> GenWebDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenWebDockerfile(self.options)

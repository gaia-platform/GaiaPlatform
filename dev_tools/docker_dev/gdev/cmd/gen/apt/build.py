"""
Module to satisfy the build requirements to generate the dockerfile for the APT section.
"""
from .dockerfile import GenAptDockerfile
from .._abc.build import GenAbcBuild


class GenAptBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the APT section.
    """

    @property
    def dockerfile(self) -> GenAptDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenAptDockerfile(self.options)

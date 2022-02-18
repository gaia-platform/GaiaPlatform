"""
Module to satisfy the push requirements to for the ENV section.
"""
from .build import GenEnvBuild
from .._abc.push import GenAbcPush


class GenEnvPush(GenAbcPush):
    """
    Class to satisfy the push requirements to for the ENV section.
    """

    @property
    def build(self) -> GenEnvBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenEnvBuild(self.options)

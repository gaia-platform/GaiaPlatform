from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenPreRunDockerfile
from .._abc.build import GenAbcBuild


class GenPreRunBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenPreRunDockerfile:
        return GenPreRunDockerfile(self.options)

from .dockerfile import GenPipDockerfile
from .._abc.build import GenAbcBuild


class GenPipBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenPipDockerfile:
        return GenPipDockerfile(self.options)

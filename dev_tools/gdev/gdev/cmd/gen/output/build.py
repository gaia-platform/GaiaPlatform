from .dockerfile import GenOutputDockerfile
from .._abc.build import GenAbcBuild


class GenOutputBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenOutputDockerfile:
        return GenOutputDockerfile(self.options)

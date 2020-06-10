from .dockerfile import GenAptDockerfile
from .._abc.build import GenAbcBuild


class GenAptBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenAptDockerfile:
        return GenAptDockerfile(self.options)

from .dockerfile import GenRunDockerfile
from .._abc.build import GenAbcBuild


class GenRunBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenRunDockerfile:
        return GenRunDockerfile(self.options)

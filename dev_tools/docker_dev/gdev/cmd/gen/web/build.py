from .dockerfile import GenWebDockerfile
from .._abc.build import GenAbcBuild


class GenWebBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenWebDockerfile:
        return GenWebDockerfile(self.options)

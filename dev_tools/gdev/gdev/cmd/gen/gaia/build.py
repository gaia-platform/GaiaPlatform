from .dockerfile import GenGaiaDockerfile
from .._abc.build import GenAbcBuild


class GenGaiaBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenGaiaDockerfile:
        return GenGaiaDockerfile(self.options)

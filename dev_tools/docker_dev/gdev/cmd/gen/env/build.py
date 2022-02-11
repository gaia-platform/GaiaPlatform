from .dockerfile import GenEnvDockerfile
from .._abc.build import GenAbcBuild


class GenEnvBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenEnvDockerfile:
        return GenEnvDockerfile(self.options)

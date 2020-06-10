from .dockerfile import GenEnvDockerfile
from .._abc.image import GenAbcImage


class GenEnvImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenEnvDockerfile:
        return GenEnvDockerfile(self.options)

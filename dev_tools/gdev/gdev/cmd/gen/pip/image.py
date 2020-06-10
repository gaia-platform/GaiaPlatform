from .dockerfile import GenPipDockerfile
from .._abc.image import GenAbcImage


class GenPipImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenPipDockerfile:
        return GenPipDockerfile(self.options)

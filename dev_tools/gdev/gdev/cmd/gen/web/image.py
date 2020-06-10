from .dockerfile import GenWebDockerfile
from .._abc.image import GenAbcImage


class GenWebImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenWebDockerfile:
        return GenWebDockerfile(self.options)

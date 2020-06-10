from .image import GenEnvImage
from .._abc.shell import GenAbcShell


class GenPreBuildShell(GenAbcShell):

    @property
    def image(self) -> GenEnvImage:
        return GenEnvImage(self.options)

from .image import GenPipImage
from .._abc.shell import GenAbcShell


class GenPipShell(GenAbcShell):

    @property
    def image(self) -> GenPipImage:
        return GenPipImage(self.options)

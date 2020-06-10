from .image import GenGaiaImage
from .._abc.shell import GenAbcShell


class GenGaiaShell(GenAbcShell):

    @property
    def image(self) -> GenGaiaImage:
        return GenGaiaImage(self.options)

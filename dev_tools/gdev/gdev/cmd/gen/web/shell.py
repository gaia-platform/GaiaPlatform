from .image import GenWebImage
from .._abc.shell import GenAbcShell


class GenWebShell(GenAbcShell):

    @property
    def image(self) -> GenWebImage:
        return GenWebImage(self.options)

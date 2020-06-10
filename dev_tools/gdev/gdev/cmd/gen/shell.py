from .image import GenImage
from ._abc.shell import GenAbcShell


class GenShell(GenAbcShell):

    @property
    def image(self) -> GenImage:
        return GenImage(self.options)

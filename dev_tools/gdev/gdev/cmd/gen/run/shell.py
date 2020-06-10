from .image import GenRunImage
from .._abc.shell import GenAbcShell


class GenRunShell(GenAbcShell):

    @property
    def image(self) -> GenRunImage:
        return GenRunImage(self.options)

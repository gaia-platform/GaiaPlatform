from .image import GenAptImage
from .._abc.shell import GenAbcShell


class GenAptShell(GenAbcShell):

    @property
    def image(self) -> GenAptImage:
        return GenAptImage(self.options)


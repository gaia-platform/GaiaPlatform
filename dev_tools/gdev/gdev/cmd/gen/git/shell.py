from .._abc.image import GenAbcImage
from .._abc.shell import GenAbcShell


class GenGitShell(GenAbcShell):

    @property
    def image(self) -> GenAbcImage:
        from .image import GenGitImage
        return GenGitImage(self.options)


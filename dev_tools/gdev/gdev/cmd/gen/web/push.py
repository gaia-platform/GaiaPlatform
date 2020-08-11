from .build import GenWebBuild
from .._abc.push import GenAbcPush


class GenWebPush(GenAbcPush):

    @property
    def build(self) -> GenWebBuild:
        return GenWebBuild(self.options)

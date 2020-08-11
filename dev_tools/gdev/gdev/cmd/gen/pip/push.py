from .build import GenPipBuild
from .._abc.push import GenAbcPush


class GenPipPush(GenAbcPush):

    @property
    def build(self) -> GenPipBuild:
        return GenPipBuild(self.options)

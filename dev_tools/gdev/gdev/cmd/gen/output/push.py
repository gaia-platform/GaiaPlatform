from .build import GenOutputBuild
from .._abc.push import GenAbcPush


class GenOutputPush(GenAbcPush):

    @property
    def build(self) -> GenOutputBuild:
        return GenOutputBuild(self.options)

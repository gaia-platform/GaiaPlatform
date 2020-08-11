from .build import GenAptBuild
from .._abc.push import GenAbcPush


class GenAptPush(GenAbcPush):

    @property
    def build(self) -> GenAptBuild:
        return GenAptBuild(self.options)

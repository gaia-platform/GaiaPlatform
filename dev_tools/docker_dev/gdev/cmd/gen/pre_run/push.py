from .build import GenPreRunBuild
from .._abc.push import GenAbcPush


class GenPreRunPush(GenAbcPush):

    @property
    def build(self) -> GenPreRunBuild:
        return GenPreRunBuild(self.options)

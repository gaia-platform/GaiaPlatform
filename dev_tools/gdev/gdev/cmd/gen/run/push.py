from .build import GenRunBuild
from .._abc.push import GenAbcPush


class GenRunPush(GenAbcPush):

    @property
    def build(self) -> GenRunBuild:
        return GenRunBuild(self.options)

from .build import GenEnvBuild
from .._abc.push import GenAbcPush


class GenEnvPush(GenAbcPush):

    @property
    def build(self) -> GenEnvBuild:
        return GenEnvBuild(self.options)

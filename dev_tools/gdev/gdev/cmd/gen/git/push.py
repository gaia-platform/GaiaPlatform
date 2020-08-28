from .build import GenGitBuild
from .._abc.push import GenAbcPush


class GenGitPush(GenAbcPush):

    @property
    def build(self) -> GenGitBuild:
        return GenGitBuild(self.options)

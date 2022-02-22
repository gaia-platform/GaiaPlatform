from .._abc.build import GenAbcBuild
from .._abc.run import GenAbcRun


class GenGitRun(GenAbcRun):

    @property
    def build(self) -> GenAbcBuild:
        from .build import GenGitBuild
        return GenGitBuild(self.options)

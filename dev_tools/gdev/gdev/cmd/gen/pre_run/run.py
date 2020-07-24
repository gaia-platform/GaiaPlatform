from .build import GenPreRunBuild
from .._abc.run import GenAbcRun


class GenPreRunRun(GenAbcRun):

    @property
    def build(self) -> GenPreRunBuild:
        return GenPreRunBuild(self.options)

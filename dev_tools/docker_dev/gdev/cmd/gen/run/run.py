from .build import GenRunBuild
from .._abc.run import GenAbcRun


class GenRunRun(GenAbcRun):

    @property
    def build(self) -> GenRunBuild:
        return GenRunBuild(self.options)

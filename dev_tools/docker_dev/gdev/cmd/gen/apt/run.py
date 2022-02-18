from .build import GenAptBuild
from .._abc.run import GenAbcRun


class GenAptRun(GenAbcRun):

    @property
    def build(self) -> GenAptBuild:
        return GenAptBuild(self.options)

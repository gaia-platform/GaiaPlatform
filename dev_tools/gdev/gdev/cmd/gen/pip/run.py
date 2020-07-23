from .build import GenPipBuild
from .._abc.run import GenAbcRun


class GenPipRun(GenAbcRun):

    @property
    def build(self) -> GenPipBuild:
        return GenPipBuild(self.options)

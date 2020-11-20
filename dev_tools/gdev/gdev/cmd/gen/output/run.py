from .build import GenOutputBuild
from .._abc.run import GenAbcRun


class GenOutputRun(GenAbcRun):

    @property
    def build(self) -> GenOutputBuild:
        return GenOutputBuild(self.options)
